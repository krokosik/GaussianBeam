// Generated from 1_1_to_1_2.xsl by convertxslt.sh. Do not edit manually
QString xslt_1_1_to_1_2("\
<?xml version=\"1.0\"?>\
<xsl:stylesheet version=\"2.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\
\
<!-- Convert GaussianBeam 1.1 files to 1.2 files -->\
\
<xsl:output  method=\"xml\" indent=\"yes\"/>\
\
<!-- Default -->\
\
<xsl:template match=\"*\">\
	<xsl:copy-of select=\".\"/>\
</xsl:template>\
\
<!-- TODO: flip mirrors, inputBeam -> createBeam -->\
\
<!-- Root -->\
\
<xsl:template match=\"gaussianBeam[@version = '1.1']\">\
<gaussianBeam version=\"1.2\">\
	<xsl:apply-templates/>\
</gaussianBeam>\
</xsl:template>\
\
</xsl:stylesheet>\
");
