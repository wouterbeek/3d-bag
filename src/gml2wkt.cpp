#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include <gdal/ogrsf_frmts.h>

void export_bag(const std::string& inputFile, std::ostream& os);
void export_geometry(std::ostream& os, const char* pandId, OGRGeometry* geometry, const std::string& crs);
const char* get_pand_id(OGRFeatureDefn* featureDef, OGRFeature* feature);
void help();
char* replace_double_quotes(const char*);

int main(int argc, char** argv)
{
  int flag;
  while ((flag = getopt(argc, argv, "h")) != -1) {
    switch (flag) {
    case 'h':
      help();
      return 0;
    default:
      std::cerr << "Unknow option.\n";
      return 1;
    }
  }
  if (argc-optind < 2) {
    std::cerr << "Missing input and/or output file name.\n";
    return 2;
  }
  const std::string inputFile {argv[optind]}, outputFile {argv[optind+1]};
  std::ofstream ofs {outputFile};
  try {
    export_bag(inputFile, ofs);
  } catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    return 3;
  } catch (...) {
    std::cerr << "An unknown error occurred.\n";
    return 3;
  }
  return 0;
}

void export_bag(const std::string& inputFile, std::ostream& os)
{
  os << "prefix geo: <http://www.opengis.net/ont/geosparql#>\n"
     << "prefix def: <https://data.labs.pdok.nl/bag/def/>\n"
     << "prefix pand: <http://bag.basisregistraties.overheid.nl/bag/id/pand/>\n\n";
  // init
  GDALAllRegister();
  // dataset
  GDALDataset* dataset = (GDALDataset*)GDALOpenEx(inputFile.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
  if (dataset == nullptr) {
    throw std::runtime_error("Opening input file failed.");
  }
  // layer
  OGRLayer* layer = dataset->GetLayer(0);
  layer->ResetReading();
  // feature
  OGRFeature* feature;
  OGRFeatureDefn* featureDef = layer->GetLayerDefn();
  OGRSpatialReference source {OGRSpatialReference(nullptr)};
  source.SetFromUserInput("EPSG:28992");
  OGRSpatialReference target {OGRSpatialReference(nullptr)};
  target.SetFromUserInput("EPSG:4326");
  OGRCoordinateTransformation* transformation = OGRCreateCoordinateTransformation(&source, &target);
  if (transformation == nullptr) {
    throw std::runtime_error("Could not create CRS transformation.");
  }
  while((feature = layer->GetNextFeature()) != nullptr) {
    const char* pandId = get_pand_id(featureDef, feature);
    OGRGeometry* geometry = feature->GetGeometryRef();
    export_geometry(os, pandId, geometry, "http://www.opengis.net/def/crs/EPSG/0/28992");
    if (geometry->transform(transformation) != OGRERR_NONE) {
      throw std::runtime_error("Could not transform from EPSG:28992 (RD) to EPSG:4326 (WGS84).");
    }
    export_geometry(os, pandId, geometry, "http://www.opengis.net/def/crs/EPSG/0/4326");
  }
  GDALClose(dataset);
}

void export_geometry(std::ostream& os, const char* pandId, OGRGeometry* geometry, const std::string& crs)
{
  if (geometry == nullptr) {
    throw std::runtime_error("Invalid geometry encountered.");
  }
  char* wkt = nullptr;
  if (geometry->exportToWkt(&wkt, wkbVariantIso) != OGRERR_NONE) {
    throw std::runtime_error("Could not export geometry as WKT string.");
  }
  const char* const* options = nullptr;
  const char* gml_tmp = geometry->exportToGML(options);
  char* gml = replace_double_quotes(gml_tmp);
  os << "pand:" << pandId << '\n'
     << "  geo:hasGeometry [\n"
     << "    def:crs <" << crs << ">;\n"
     << "    geo:asGML \"" << gml << "\"^^geo:gmlLiteral;\n"
     << "    geo:asWKT \"";
  if (crs != "http://www.opengis.net/def/crs/EPSG/0/4326") {
    os << '<' << crs << "> ";
  }
  os << wkt << "\"^^geo:wktLiteral;\n"
     << "    geo:dimension 3 ].\n";
  std::free(gml);
  CPLFree(wkt);
}

const char* get_pand_id(OGRFeatureDefn* featureDef, OGRFeature* feature)
{
  for(int i {0}; i < featureDef->GetFieldCount(); i++) {
    OGRFieldDefn* fieldDef = featureDef->GetFieldDefn(i);
    if (std::strcmp(fieldDef->GetNameRef(),"gml_id") == 0) {
      if (fieldDef->GetType() != OFTString) {
        throw std::runtime_error("Unanticipated field value.");
      }
      return feature->GetFieldAsString(i);
    }
  }
  throw std::runtime_error("Could not find pand ID.");
}

void help()
{
  std::cout << "$ gml2wkt [options] <input-file> <output-file>\n"
            << "\t-h\t\t\tDisplay this help message.\n\n"
            << "Converts the GML <input-file> with BAG buildings in RD into the <output-file> containing Linked Data, using both GML and WKT, and using both RD and WGS84.\n\n";
}

char* replace_double_quotes(const char* s)
{
  char* t = (char*)std::malloc(sizeof(char)*2*std::strlen(s));
  unsigned i {0};
  while (*s != '\0') {
    if (*s == '"') {
      t[i++] = '\\';
    }
    t[i++] = *s;
    s += sizeof(char);
  }
  t[i] = '\0';
  return t;
}
