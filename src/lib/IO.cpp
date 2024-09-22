#include "IO.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxslt/transform.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>

class xmlchar_helper
{
public:
    xmlchar_helper(xmlChar *ptr) : ptr_(ptr) {}

    ~xmlchar_helper()
        { if (ptr_) xmlFree(ptr_); }

    const char* get() const
        { return reinterpret_cast<const char*>(ptr_); }

private:
    xmlChar *ptr_;
};

void convertFormat(std::string &data, std::string const xsl_path) {
  // Load the XML from the input data
  xmlDocPtr inputDoc = xmlParseMemory(data.c_str(), data.size());
  if (inputDoc == nullptr) {
    // Handle XML parsing error
    return;
  }

  xmlDocPtr xsltDoc = xmlParseMemory(xsl_path.c_str(), xsl_path.size());
  if (xsltDoc == nullptr) {
    // Handle XSLT parsing error
    xmlFreeDoc(inputDoc);
    return;
  }

  xsltStylesheetPtr xslt = xsltParseStylesheetDoc(xsltDoc);
  if (xslt == nullptr) {
    // Handle XSLT parsing error
    xmlFreeDoc(inputDoc);
    xmlFreeDoc(xsltDoc);
    return;
  }

  // Apply the XSLT transformation
  xmlDocPtr resultDoc = xsltApplyStylesheet(xslt, inputDoc, nullptr);
  if (resultDoc == nullptr) {
    // Handle transformation error
    xsltFreeStylesheet(xslt);
    xmlFreeDoc(inputDoc);
    xmlFreeDoc(xsltDoc);
    return;
  }
  
  // Convert the transformed XML to QByteArray
  xmlChar *resultBuffer = nullptr;
  int resultSize = 0;
  xsltSaveResultToString(&resultBuffer, &resultSize, resultDoc, xslt);

  xmlchar_helper helper(resultBuffer);

  data.assign(helper.get());

  // Clean up
  xsltFreeStylesheet(xslt);
  xmlFreeDoc(inputDoc);
  xmlFreeDoc(resultDoc);
}
