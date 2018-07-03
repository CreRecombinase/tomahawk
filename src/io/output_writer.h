#ifndef IO_OUTPUT_WRITER_H_
#define IO_OUTPUT_WRITER_H_

#include "support/magic_constants.h"
#include "support/simd_definitions.h"
#include "support/helpers.h"
#include "algorithm/spinlock.h"
#include "compression/tgzf_controller.h"
#include "index/tomahawk_header.h"
#include "index/index_entry.h"
#include "containers/output_container.h"
#include "tomahawk/output_entry_support.h"
#include "tomahawk/meta_entry.h"
#include "index/index_entry.h"
#include "index/index_container.h"
#include "index/index.h"
#include "index/footer.h"

namespace tomahawk{
namespace io{

class OutputWriterInterface{
protected:
	typedef OutputWriterInterface           self_type;
	typedef TGZFController                  compression_type;
	typedef algorithm::SpinLock             spin_lock_type;
	typedef BasicBuffer                     buffer_type;
	typedef TomahawkHeader                  twk_header_type;
	typedef totempole::IndexEntry           index_entry_type;
	typedef OutputEntry                     entry_type;
	typedef support::OutputEntrySupport     entry_support_type;
	typedef totempole::IndexEntry           header_entry_type;
	typedef totempole::IndexContainer       index_container_type;
	typedef Index                           index_type;
	typedef totempole::Footer               footer_type;
	typedef size_t                          size_type;
	typedef containers::OutputContainer     container_type;

public:
	OutputWriterInterface(void);
	OutputWriterInterface(const self_type& other);
	virtual ~OutputWriterInterface(void);

	inline const U64& sizeEntries(void) const{ return(this->n_entries); }
	inline const U32& sizeBlocks(void) const{ return(this->n_blocks); }

	inline const U64& totalBytesAdded(void) const{ return(this->bytes_added); }
	inline const U64& totalBytesWritten(void) const{ return(this->bytes_written); }
	// Setters
	inline void setSorted(const bool yes){ this->writing_sorted_ = yes; }
	inline void setPartialSorted(const bool yes){ this->writing_sorted_partial_ = yes; }
	inline void setFlushLimit(const U32 limit){ this->l_flush_limit = limit; }

	// Getters
	inline const bool isSorted(void) const{ return(this->writing_sorted_); }
	inline const bool isPartialSorted(void) const{ return(this->writing_sorted_partial_); }

	// Get
	inline index_type* getIndex(void){ return(this->index_); }

	// Progress
	inline void ResetProgress(void){ this->n_progress_count = 0; }
	inline const U32& getProgressCounts(void) const{ return this->n_progress_count; }

	inline self_type& operator+=(const self_type& other){
		this->n_entries += other.n_entries;
		this->n_blocks  += other.n_blocks;
		if(other.l_largest_uncompressed > this->l_largest_uncompressed)
			this->l_largest_uncompressed = other.l_largest_uncompressed;
		this->bytes_written += other.bytes_written;
		this->bytes_added   += other.bytes_added;


		return(*this);
	}

	inline self_type& operator=(const self_type& other){
		this->n_blocks  = other.n_blocks;
		this->n_entries = other.n_entries;
		if(other.l_largest_uncompressed > this->l_largest_uncompressed)
			this->l_largest_uncompressed = other.l_largest_uncompressed;
		this->bytes_added   = other.bytes_added;
		this->bytes_written = other.bytes_written;
		return(*this);
	}

	virtual void operator<<(const entry_type& entry) =0;

	//
	virtual bool open(void) =0;
	virtual bool open(const std::string& output_file) =0;
	virtual int writeHeaders(twk_header_type& twk_header) =0;
	virtual void writeFinal(void) =0;
	virtual void flush(const bool lock = true) =0;

	inline void setUpperOnly(const bool set){ this->upper_only_ = set; }
	inline const bool& hasUpperOnly(void) const{return(this->upper_only_); }

	/**<
	 * Primary function writing `two` entries to disk after being computed by a
	 * slave.
	 * @param meta_a   Meta information for the from container
	 * @param meta_b   Meta information for the to container
	 * @param header_a Tomahawk index entry for the from container
	 * @param header_b Tomahawk index entry for the to container
	 * @param helper   Helper structure used in computing LD. Holds the allele/genotype counts and statistics
	 */
	virtual void add(const MetaEntry& meta_a, const MetaEntry& meta_b, const header_entry_type& header_a, const header_entry_type& header_b, const entry_support_type& helper) =0;

	/**<
	 * Overloaded operator for adding an entire container of `two` entries
	 * @param container Target container of entries
	 */
	virtual void operator<<(const container_type& container) =0;

	/**<
	 * Overloaded operator for adding an entire buffer of `two` entries
	 * @param buffer Target buffer of entries
	 */
	virtual void operator<<(buffer_type& buffer) =0;


protected:
	bool             owns_pointers;
	bool             writing_sorted_;
	bool             writing_sorted_partial_;
	bool             upper_only_;
	U64              n_entries;        // number of entries written
	U32              n_progress_count; // lines added since last flush
	U32              n_blocks;         // number of index blocks writtenflush_limit
	U32              l_flush_limit;
	U32              l_largest_uncompressed;
	U64              bytes_added;
	U64              bytes_written;
	index_entry_type index_entry;      // keep track of sort order
	buffer_type      buffer;
	compression_type compressor;
	spin_lock_type*  spin_lock;
	index_type*      index_;
	footer_type*     footer_;
};

/**<
 * Writer class for `two` entries. This class supports parallel writing
 * of atomic values). In parallel computing, each slave constructs their own
 * OutputWriter by invoking the copy-ctor and borrowing pointers from the
 * main instance.
 */
class OutputWriterBinaryFile : public OutputWriterInterface{
private:
	typedef OutputWriterInterface parent_type;
	typedef OutputWriterBinaryFile      self_type;

public:
	OutputWriterBinaryFile(void);
	OutputWriterBinaryFile(std::string input_file);
	OutputWriterBinaryFile(const self_type& other);
	~OutputWriterBinaryFile(void);

	bool open(void){ return false; }
	bool open(const std::string& output_file);
	int writeHeaders(twk_header_type& twk_header);
	void writeFinal(void);
	void flush(const bool lock = true);


	void add(const MetaEntry& meta_a, const MetaEntry& meta_b, const header_entry_type& header_a, const header_entry_type& header_b, const entry_support_type& helper);

	/**<
	 * Overloaded operator for adding a single `two` entry
	 * @param entry Input `two` entry
	 */
	inline void operator<<(const entry_type& entry){
		if(this->index_entry.n_variants == 0){
			this->index_entry.contigID     = entry.AcontigID;
			this->index_entry.min_position = entry.Aposition;
			this->index_entry.max_position = entry.Aposition;
		}

		if(this->index_entry.contigID != entry.AcontigID){
			this->flush();
			this->index_entry.contigID     = entry.AcontigID;
			this->index_entry.min_position = entry.Aposition;
			this->index_entry.max_position = entry.Aposition;
		}

		// Check if the buffer has to be flushed after adding this entry
		if(this->buffer.size() > this->l_flush_limit){
			this->flush();
			this->index_entry.contigID     = entry.AcontigID;
			this->index_entry.min_position = entry.Aposition;
			this->index_entry.max_position = entry.Aposition;
		}

		this->buffer << entry;
		++this->n_entries;
		++this->index_entry;
		this->index_entry.max_position = entry.Aposition;
	}

	/**<
	 * Overloaded operator for adding an entire container of `two` entries
	 * @param container Target container of entries
	 */
	void operator<<(const container_type& container);

	/**<
	 * Overloaded operator for adding an entire buffer of `two` entries
	 * @param buffer Target buffer of entries
	 */
	void operator<<(buffer_type& buffer);

	void writePrecompressedBlock(buffer_type& buffer, const U64& uncompressed_size);

private:
	void CheckOutputNames(const std::string& input);

private:
	std::string      filename;
	std::string      basePath;
	std::string      baseName;
	std::ofstream*   stream;
};

class OutputWriterBinaryStream : public OutputWriterInterface{
private:
	typedef OutputWriterInterface    parent_type;
	typedef OutputWriterBinaryStream self_type;

public:
	OutputWriterBinaryStream(void) = default;
	~OutputWriterBinaryStream(void) = default;

	bool open(void){ return true; }
	bool open(const std::string& output_file){ return false; }
	int writeHeaders(twk_header_type& twk_header);
	void writeFinal(void);
	void flush(const bool lock = true);


	void add(const MetaEntry& meta_a, const MetaEntry& meta_b, const header_entry_type& header_a, const header_entry_type& header_b, const entry_support_type& helper);

	/**<
	 * Overloaded operator for adding a single `two` entry
	 * @param entry Input `two` entry
	 */
	inline void operator<<(const entry_type& entry){
		if(this->index_entry.n_variants == 0){
			this->index_entry.contigID     = entry.AcontigID;
			this->index_entry.min_position = entry.Aposition;
			this->index_entry.max_position = entry.Aposition;
		}

		if(this->index_entry.contigID != entry.AcontigID){
			this->flush();
			this->index_entry.contigID     = entry.AcontigID;
			this->index_entry.min_position = entry.Aposition;
			this->index_entry.max_position = entry.Aposition;
		}

		// Check if the buffer has to be flushed after adding this entry
		if(this->buffer.size() > this->l_flush_limit){
			this->flush();
			this->index_entry.contigID     = entry.AcontigID;
			this->index_entry.min_position = entry.Aposition;
			this->index_entry.max_position = entry.Aposition;
		}

		this->buffer << entry;
		++this->n_entries;
		++this->index_entry;
		this->index_entry.max_position = entry.Aposition;
	}

	/**<
	 * Overloaded operator for adding an entire container of `two` entries
	 * @param container Target container of entries
	 */
	void operator<<(const container_type& container);

	/**<
	 * Overloaded operator for adding an entire buffer of `two` entries
	 * @param buffer Target buffer of entries
	 */
	void operator<<(buffer_type& buffer);

	void writePrecompressedBlock(buffer_type& buffer, const U64& uncompressed_size);

private:
	//int64_t     virtual_file_offset_;
};

class OutputWriterStdOut : public OutputWriterInterface{
private:
	typedef OutputWriterInterface parent_type;
	typedef OutputWriterStdOut    self_type;

public:
	OutputWriterStdOut() = default;
	~OutputWriterStdOut() = default;

	bool open(void){ return true; }
	bool open(const std::string& output_file){ return true; }

	int writeHeaders(twk_header_type& twk_header){
		// write static headers
		std::cout << twk_header.getLiterals() << '\n';
		std::cout << "FLAG\tCHROM_A\tPOS_A\tCHROM_B\tPOS_B\tREF_REF\tREF_ALT\tALT_REF\tALT_ALT\tD\tDprime\tR\tR2\tP\tChiSqModel\tChiSqTable";
		std::cout << std::endl;
		return(0);
	}

	void writeFinal(void){}

	void flush(const bool lock = true){
		if(this->buffer.size() > 0){
			this->bytes_added   += this->buffer.size();
			this->bytes_written += this->buffer.size();

			if(lock) this->spin_lock->lock();
			std::cout.write(this->buffer.data(), this->buffer.size());
			++this->n_blocks;
			if(lock) this->spin_lock->unlock();

			this->buffer.reset();
		}
	}

	/**<
	 * Primary function writing `two` entries to stdout after being computed by a
	 * slave.
	 * @param meta_a   Meta information for the from container
	 * @param meta_b   Meta information for the to container
	 * @param header_a Tomahawk index entry for the from container
	 * @param header_b Tomahawk index entry for the to container
	 * @param helper   Helper structure used in computing LD. Holds the allele/genotype counts and statistics
	 */
	void add(const MetaEntry& meta_a, const MetaEntry& meta_b, const header_entry_type& header_a, const header_entry_type& header_b, const entry_support_type& helper){
		this->buffer.AddReadble(helper.controller); this->buffer += '\t';
		this->buffer.AddReadble(header_a.contigID); this->buffer += '\t';
		this->buffer.AddReadble(meta_a.position);   this->buffer += '\t';
		this->buffer.AddReadble(header_b.contigID); this->buffer += '\t';
		this->buffer.AddReadble(meta_b.position);   this->buffer += '\t';
		helper.addReadble(this->buffer);            this->buffer += '\n';

		if(this->upper_only_ == false){
			this->buffer.AddReadble(helper.controller); this->buffer += '\t';
			this->buffer.AddReadble(header_b.contigID); this->buffer += '\t';
			this->buffer.AddReadble(meta_b.position);   this->buffer += '\t';
			this->buffer.AddReadble(header_a.contigID); this->buffer += '\t';
			this->buffer.AddReadble(meta_a.position);   this->buffer += '\t';
			helper.addReadble(this->buffer);            this->buffer += '\n';

			this->n_entries += 1;
			this->n_progress_count += 1;
			this->index_entry.n_variants += 1;
		}

		this->n_entries += 1;
		this->n_progress_count += 1;
		this->index_entry.n_variants += 1;

		if(this->buffer.size() > this->l_flush_limit)
			this->flush();
	}

	inline void operator<<(const entry_type& entry){
		if(this->index_entry.n_variants == 0){
			this->index_entry.contigID     = entry.AcontigID;
			this->index_entry.min_position = entry.Aposition;
			this->index_entry.max_position = entry.Aposition;
		}

		if(this->index_entry.contigID != entry.AcontigID){
			this->flush();
			this->index_entry.contigID     = entry.AcontigID;
			this->index_entry.min_position = entry.Aposition;
			this->index_entry.max_position = entry.Aposition;
		}

		// Check if the buffer has to be flushed after adding this entry
		if(this->buffer.size() > this->l_flush_limit){
			this->flush();
			this->index_entry.contigID     = entry.AcontigID;
			this->index_entry.min_position = entry.Aposition;
			this->index_entry.max_position = entry.Aposition;
		}

		this->buffer << entry;
		++this->n_entries;
		++this->index_entry;
		this->index_entry.max_position = entry.Aposition;
	}

	void operator<<(const container_type& container){
		std::cerr << "Cannot write processed binary block to cout..." << std::endl;
		exit(1);
	}

	void operator<<(buffer_type& buffer){
		std::cerr << "Cannot write processed binary block to cout..." << std::endl;
		exit(1);
	}
};

}
}

#endif /* IO_OUTPUT_WRITER_H_ */