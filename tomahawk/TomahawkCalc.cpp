
#include "TomahawkCalc.h"

namespace Tomahawk {

bool TomahawkCalc::Open(const std::string input){

}

bool TomahawkCalc::OpenWriter(void){
	if(this->writer == nullptr)
		this->SelectWriterOutputType(IO::GenericWriterInterace::type::cout);

	return(this->writer->open());
}


bool TomahawkCalc::OpenWriter(const std::string destination){
	if(this->writer == nullptr){
		if(destination == "-"){
			this->SelectWriterOutputType(IO::GenericWriterInterace::type::cout);
			return(this->writer->open());
		}
		this->SelectWriterOutputType(IO::GenericWriterInterace::type::file);
	}

	return(this->writer->open(destination));
}

template <typename K, typename V>
bool comparePairs(const std::pair<K,V>& a, const std::pair<K,V>& b){ return a.first < b.first; }

bool TomahawkCalc::CalculateWrapper(){
	const BYTE bit_width = this->reader.getBitWidth();
	if(bit_width == 1) 	    return(this->Calculate<BYTE>());
	else if(bit_width == 2) return(this->Calculate<U16>());
	else if(bit_width == 4) return(this->Calculate<U32>());
	else if(bit_width == 8) return(this->Calculate<U64>());
	else {
		std::cerr << Helpers::timestamp("ERROR", "TOMAHAWK") << "Impossible bit width..." << std::endl;
		exit(1);
	}

	//std::cerr << "Manager have: " << this->manager_.size() << std::endl;
	//return(this->manager_.AllVersusAll());
	return false;
}

bool TomahawkCalc::Calculate(std::vector< std::pair<U32,U32> >& blocks){
	// Todo!
	std::sort(blocks.begin(), blocks.end(), comparePairs<U32, U32>);
	if(!this->reader.getBlocks(blocks)){
		std::cerr << Helpers::timestamp("ERROR", "TOMAHAWK") << "Failed to get Tomahawk blocks..." << std::endl;
		return false;
	}

	return(this->CalculateWrapper());
}

bool TomahawkCalc::Calculate(std::vector<U32>& blocks){
	if(!this->reader.getBlocks(blocks)){
		std::cerr << Helpers::timestamp("ERROR", "TOMAHAWK") << "Failed to get Tomahawk blocks..." << std::endl;
		return false;
	}

	if(!SILENT)
		std::cerr << Helpers::timestamp("LOG","TOMAHAWK") << "Inflated " << blocks.size() << " blocks..." << std::endl;

	return(this->CalculateWrapper());
}

bool TomahawkCalc::Calculate(){
	if(!this->balancer.Build(this->reader.getTotempole().getBlocks(), this->threads)){
		std::cerr << Helpers::timestamp("ERROR", "BALANCER") << "Failed to split into blocks..." << std::endl;
		return false;
	}

	return(this->Calculate(this->balancer.getLoad()));
}

bool TomahawkCalc::SelectWriterOutputType(const IO::GenericWriterInterace::type writer_type){
	if(this->writer != nullptr)
		return false;

	if(writer_type == IO::GenericWriterInterace::type::cout)
		this->writer = new IO::WriterStandardOut;
	else
		this->writer = new IO::WriterFile;

	return true;
}

bool TomahawkCalc::WriteTwoHeader(void){
	if(this->parameters.compression_type == IO::GenericWriterInterace::compression::natural)
		return(this->WriteTwoHeaderNatural());
	else
		return(this->WriteTwoHeaderBinary());
}

bool TomahawkCalc::WriteTwoHeaderNatural(void){
	std::ostream& stream = this->writer->getStream();
	stream << "FLAG\tSCORE\tcontigA\tpositionA\tcontigB\tpositionB\tp11\tp12\tp21\tp22\tD\tDprime\tRsquared\tPFisher\tChiSquaredCV\tPmodel" << std::endl;
	return true;
}

bool TomahawkCalc::WriteTwoHeaderBinary(void){
	std::ostream& stream = this->writer->getStream();
	// Stream needs to be ofstream for overloading to function properly
	std::ofstream& streamRe = *reinterpret_cast<std::ofstream*>(&stream);

	streamRe.write(Constants::WRITE_HEADER_LD_MAGIC, Constants::WRITE_HEADER_LD_MAGIC_LENGTH);

	const totempole_reader& totempole = this->reader.getTotempole();
	const U64& samples = totempole.getSamples();
	Totempole::TotempoleHeader h(samples);
	streamRe << h;

	// Write out dummy variable for IO offset
	U32 nothing = 0; // Dummy variable
	size_t posOffset = streamRe.tellp(); // remember current IO position
	streamRe.write(reinterpret_cast<const char*>(&nothing), sizeof(U32)); // data offset

	// Write the number of contigs
	const U32 n_contigs = totempole.getContigs();
	streamRe.write(reinterpret_cast<const char*>(&n_contigs), sizeof(U32));

	// Write contig data to TWO
	// length | n_char | chars[0 .. n_char - 1]
	for(U32 i = 0; i < totempole.getContigs(); ++i)
		streamRe << *totempole.getContigBase(i);

	U32 curPos = streamRe.tellp(); // remember current IO position
	streamRe.seekp(posOffset); // seek to previous position
	streamRe.write(reinterpret_cast<const char*>(&curPos), sizeof(U32)); // overwrite data offset
	streamRe.seekp(curPos); // seek back to current IO position

	return(streamRe.good());
}

} /* namespace Tomahawk */