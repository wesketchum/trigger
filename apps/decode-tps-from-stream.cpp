#include <iostream>
#include <fstream>
#include <triggeralgs/TriggerPrimitive.hpp>

int
main(int argc, char** argv)
{
  using triggeralgs::TriggerPrimitive;
  const size_t TP_SIZE = sizeof(TriggerPrimitive);
  
  std::ifstream fin(argv[1], std::ios::binary);
  fin.seekg(0, fin.end);
  auto length = fin.tellg();
  fin.seekg(0, fin.beg);
  if (length == 0) {
    throw std::runtime_error("Empty file");
  }
  if (length % TP_SIZE != 0) {
    throw std::runtime_error("File does not contain an integer number of frames");
  }
  auto n_tps = length / TP_SIZE;
  // Slurp in the file
  char* buf = new char[length];
  fin.read(buf, length);

  std::ofstream fout(argv[2]);
  
  for(size_t i=0; i<n_tps; ++i){
    const TriggerPrimitive& tp = *reinterpret_cast<TriggerPrimitive*>(buf+i*TP_SIZE);
    fout << "\t" << tp.time_start << "\t" << tp.time_over_threshold << "\t" << tp.time_peak << "\t" << tp.channel << "\t" << tp.adc_integral << "\t"
         << tp.adc_peak << "\t" << tp.detid << "\t" << tp.type << "\t" << std::endl;
  }
}
