#include "postgres.h"
#include "utils/xml.h"
#include "utils/builtins.h"
#include "mb/pg_wchar.h"
#include "fmgr.h"
#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <libxml/relaxng.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <unistd.h>
#include <fcntl.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(read_text);
Datum read_text(PG_FUNCTION_ARGS) {
    text *filenameData = PG_GETARG_XML_P(0);
    char* filename = text_to_cstring(filenameData);
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        ereport(ERROR, (errcode_for_file_access(),
            errmsg("could not open file \"%s\"", filename)));
    }

    off_t size = lseek(fd, 0, SEEK_END);

    if (size == (off_t) -1) {
        ereport(ERROR, (errcode_for_file_access(),
            errmsg("could not seek in file \"%s\"", filename)));

    }
    lseek(fd, 0, SEEK_SET);

    char *resStr = (char *) palloc(size + 1);
    ssize_t bytes_read = read(fd, resStr, size);
    if (bytes_read != size) {
        ereport(ERROR, (errcode_for_file_access(),
            errmsg("could not read file \"%s\"", filename)));

    }
    close(fd);
    resStr[size] = '\0';
    text *res = cstring_to_text((char *) resStr);
    PG_RETURN_TEXT_P(res);
}

int xmlDocumentsEqual(xmlNodePtr node1, xmlNodePtr node2) {
    if (node1 == NULL && node2 == NULL)
        return 1;

    if ((node1 == NULL && node2 != NULL) || (node1 != NULL && node2 == NULL))
        return 0;

    if (node1->type != node2->type)
        return 0;

    if (node1->type == XML_TEXT_NODE && strcmp((char *)node1->content, (char *)node2->content) != 0)
        return 0;

    return xmlDocumentsEqual(node1->xmlChildrenNode, node2->xmlChildrenNode);
}

PG_FUNCTION_INFO_V1(xml_docs_equal);
Datum xml_docs_equal(PG_FUNCTION_ARGS) {
    xmltype *doc1 = PG_GETARG_XML_P(0);
    xmltype *doc2 = PG_GETARG_XML_P(1);

    char* doc1Str = text_to_cstring(doc1);
    xmlDocPtr doc1X = xmlReadMemory(doc1Str, strlen(doc1Str), "", NULL, 0);
    pfree(doc1Str);

    char* doc2Str = text_to_cstring(doc2);
    xmlDocPtr doc2X = xmlReadMemory(doc2Str, strlen(doc2Str), "", NULL, 0);
    pfree(doc2Str);

    xmlNodePtr root1 = xmlDocGetRootElement(doc1X);
    xmlNodePtr root2 = xmlDocGetRootElement(doc2X);

    int equal = xmlDocumentsEqual(root1, root2);

    xmlFreeDoc(doc1X);
    xmlFreeDoc(doc2X);
    xmlCleanupParser();

    PG_RETURN_BOOL(equal);
}

static void xsltErrorHandler(void *ctx, const char *msg, ...) {
    char error_message[1024];
    va_list args;
    va_start(args, msg);
    vsnprintf(error_message, sizeof(error_message), msg, args);
    va_end(args);

    ereport(ERROR,
            (errcode(ERRCODE_EXTERNAL_ROUTINE_EXCEPTION),
             errmsg("Error processing XSLT stylesheet: %s", error_message)));
}

PG_FUNCTION_INFO_V1(xsl_transform);
Datum xsl_transform(PG_FUNCTION_ARGS) {
    xmltype *styleData = PG_GETARG_XML_P(0);
    xmltype *srcData = PG_GETARG_XML_P(1);

    char* styleStr = text_to_cstring(styleData);
    xmlDocPtr styleDoc = xmlReadMemory(styleStr, strlen(styleStr), "", NULL, 0);
    pfree(styleStr);

    xsltSetGenericErrorFunc(NULL, xsltErrorHandler);

    xsltStylesheetPtr style = xsltParseStylesheetDoc(styleDoc);
    char* srcStr = text_to_cstring(srcData);
    xmlDocPtr src = xmlReadMemory(srcStr, strlen(srcStr), "", NULL, 0);
    pfree(srcStr);

    xsltTransformContextPtr tctxt = xsltNewTransformContext(style, src);
    xmlDocPtr resDoc = xsltApplyStylesheetUser(style, src, NULL, NULL, NULL, tctxt);
    xsltFreeTransformContext(tctxt);
    xmlFreeDoc(src);
    xsltFreeStylesheet(style);

    xmlChar *resStr;
    int buffersize;
    xmlDocDumpFormatMemory(resDoc, &resStr, &buffersize, 1);

    xmlFreeDoc(resDoc);
    xmltype *resData = cstring_to_text((char *) resStr);

    PG_RETURN_XML_P(resData);
}

PG_FUNCTION_INFO_V1(xsd_validate);
Datum xsd_validate(PG_FUNCTION_ARGS) {
    xmltype *schemaData = PG_GETARG_XML_P(0);
    xmltype *srcData = PG_GETARG_XML_P(1);

    char* schemaStr = text_to_cstring(schemaData);
    xmlSchemaParserCtxtPtr schemaCtxt = xmlSchemaNewMemParserCtxt(schemaStr, strlen(schemaStr));
    xmlSchemaPtr schema = xmlSchemaParse(schemaCtxt);
    pfree(schemaStr);
    xmlSchemaFreeParserCtxt(schemaCtxt);

    char* srcStr = text_to_cstring(srcData);
    xmlDocPtr src = xmlReadMemory(srcStr, strlen(srcStr), "", NULL, 0);
    pfree(srcStr);

    xmlSchemaValidCtxtPtr validCtxt = xmlSchemaNewValidCtxt(schema);
    int valid = xmlSchemaValidateDoc(validCtxt, src);

    // xmlSchemaFreeValidCtxt(validCtxt);
    // xmlSchemaFree(schema);
    // xmlFreeDoc(src);
    // xmlCleanupParser();

    PG_RETURN_BOOL(valid);
}

PG_FUNCTION_INFO_V1(relaxng_validate);
Datum relaxng_validate(PG_FUNCTION_ARGS) {
    xmltype *schemaData = PG_GETARG_XML_P(0);
    xmltype *srcData = PG_GETARG_XML_P(1);

    char* schemaStr = text_to_cstring(schemaData);
    xmlRelaxNGParserCtxtPtr schemaParser = xmlRelaxNGNewMemParserCtxt(schemaStr, strlen(schemaStr));
    xmlRelaxNGPtr schema = xmlRelaxNGParse(schemaParser);
    pfree(schemaStr);

    char* srcStr = text_to_cstring(srcData);
    xmlDocPtr src = xmlReadMemory(srcStr, strlen(srcStr), "", NULL, 0);
    pfree(srcStr);

    xmlRelaxNGValidCtxtPtr validCtxt = xmlRelaxNGNewValidCtxt(schema);
    int valid = xmlRelaxNGValidateDoc(validCtxt, src);

    // xmlSchemaFreeValidCtxt(validCtxt);
    // xmlSchemaFree(schema);
    // xmlFreeDoc(src);
    // xmlCleanupParser();

    PG_RETURN_BOOL(valid);
}