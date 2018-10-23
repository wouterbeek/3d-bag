#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#include <gdal/ogrsf_frmts.h>

void export_supported_driver_names(const std::string& fileName);
void help();
void print_supported_driver_names(std::ostream& os);

int main(int argc, char** argv)
{
  int flag;
  std::string fileName;
  while ((flag = getopt(argc, argv, "e:hp")) != -1) {
    switch (flag) {
    case 'e':
      fileName = std::string(optarg);
      break;
    case 'h':
      help();
      return 0;
    case 'p':
      if (fileName != "") {
        std::cerr << "The export and print flags cannot be combined.\n";
        return 1;
      }
      break;
    default:
      std::cerr << "Unknown option.\n";
      return 2;
    }
  }
  try {
    if (fileName == "") {
      print_supported_driver_names(std::cout);
    } else {
      export_supported_driver_names(fileName);
    }
  } catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    return 3;
  } catch (...) {
    std::cerr << "An unknown error occurred.\n";
    return 4;
  }
  return 0;
}

void export_supported_driver_names(const std::string& fileName)
{
  std::ofstream ofs {fileName};
  print_supported_driver_names(ofs);
}

void help()
{
  std::cout << "$ drivers [options]\n"
            << "\t-e <output-file>\t\tExport the driver names to <output-file>.\n"
            << "\t-h\t\t\tDisplay this help message.\n"
            << "\t-p\t\t\tPrint the driver names to standard output (default behavior).\n\n"
            << "Prints or exports the GDAL drivers that are supported on the current system.\n\n";
}

void print_supported_driver_names(std::ostream& os)
{
  // init
  GDALAllRegister();
  std::vector<std::string> driverNames;
  for (int i {0}; i < GetGDALDriverManager()->GetDriverCount(); i++) {
    driverNames.push_back(GetGDALDriverManager()->GetDriver(i)->GetDescription());
  }
  std::sort(driverNames.begin(), driverNames.end());
  for (std::string& driverName: driverNames) {
    os << driverName << '\n';
  }
}
