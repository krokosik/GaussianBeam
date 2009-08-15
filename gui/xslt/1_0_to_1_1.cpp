// Generated from 1_0_to_1_1.xsl by convertxslt.sh. Do not edit manually
QString xslt_1_0_to_1_1("\
<?xml version=\"1.0\"?>\
<xsl:stylesheet version=\"2.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\
\
<!-- Convert GaussianBeam 1.0 files to 1.1 files -->\
\
<xsl:output  method=\"xml\" indent=\"yes\"/>\
\
<!-- Default -->\
\
<xsl:template  match=\"*\">\
	<xsl:copy-of select=\".\"/>\
</xsl:template>\
\
<!-- Fit -->\
\
<xsl:template match=\"gaussianBeam[@version = '1.0']/waistFit\">\
	<beamFit id=\"0\">\
		<name>Fit</name>\
		<dataType><xsl:value-of select=\"fitDataType\"/></dataType>\
		<xsl:variable name=\"firstData\" select=\"position()\"/>\
		<xsl:for-each select=\"fitData\">\
			<xsl:element name=\"data\">\
			<xsl:attribute name=\"id\"><xsl:value-of select=\"position() - $firstData\"/></xsl:attribute>\
				<position><xsl:value-of select=\"dataPosition\"/></position>\
				<value><xsl:value-of select=\"dataValue\"/></value>\
			</xsl:element>\
		</xsl:for-each>\
	</beamFit>\
</xsl:template>\
\
<!-- Target beam -->\
\
<xsl:template match=\"gaussianBeam[@version = '1.0']/magicWaist\">\
	<targetBeam id=\"0\">\
		<position><xsl:value-of select=\"targetPosition\"/></position>\
		<waist><xsl:value-of select=\"targetWaist\"/></waist>\
		<xsl:apply-templates select=\"positionTolerance\"/>\
		<xsl:apply-templates select=\"waistTolerance\"/>\
	</targetBeam>\
</xsl:template>\
\
<!-- Root -->\
\
<xsl:template match=\"gaussianBeam[@version = '1.0']\">\
<gaussianBeam version=\"1.1\">\
	<bench id=\"0\">\
		<xsl:apply-templates select=\"wavelength\"/>\
		<xsl:variable name=\"horizontalRange\" select=\"display/HRange/.\"/>\
		<leftBoundary><xsl:value-of select=\"display/HOffset\"/></leftBoundary>\
		<rightBoundary><xsl:value-of select=\"display/HOffset + $horizontalRange\"/></rightBoundary>\
		<xsl:apply-templates select=\"magicWaist\"/>\
		<xsl:apply-templates select=\"waistFit\"/>\
		<opticsList>\
			<xsl:apply-templates select=\"inputBeam|lens|flatInterface|curvedInterface|flatMirror|genericABCD\"/>\
		</opticsList>\
	</bench>\
	<view id=\"0\" bench=\"0\">\
		<horizontalRange><xsl:value-of select=\"display/HRange\"/></horizontalRange>\
		<verticalRange><xsl:value-of select=\"display/VRange div 1000.\"/></verticalRange>\
		<origin><xsl:value-of select=\"display/HOffset\"/></origin>\
		<showTargetBeam id=\"0\"><xsl:value-of select=\"magicWaist/showTargetWaist\"/></showTargetBeam>\
	</view>\
</gaussianBeam>\
</xsl:template>\
\
</xsl:stylesheet>\
");
