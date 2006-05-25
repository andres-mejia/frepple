#!/usr/bin/perl
#  file     : $HeadURL: file:///develop/SVNrepository/frepple/trunk/test/runtest.pl $
#  revision : $LastChangedRevision$  $LastChangedBy$
#  date     : $LastChangedDate$
#  email    : jdetaeye@users.sourceforge.net

use frepple;

# The eval command is used to catch frepple exceptions
eval {
  print "Initializing:\n";
  FreppleInitialize();
  print " OK\n";
  
	print "Reading base data:\n";
	frepple::readXMLData('<?xml version="1.0" encoding="UTF-8" ?>
		<PLAN xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
			<NAME>actual plan</NAME>
			<DESCRIPTION>Anything goes</DESCRIPTION>
			<CURRENT>2005-01-01T00:00:01</CURRENT>
			<OPERATIONS>
				<OPERATION NAME="make end item" xsi:type="OPERATION_FIXED_TIME">
					<DURATION>24:00:00</DURATION>
				</OPERATION>
			</OPERATIONS>
			<ITEMS>
				<ITEM NAME="end item">
					<OPERATION NAME="delivery end item">
						<DURATION>24:00:00</DURATION>
					</OPERATION>
				</ITEM>
			</ITEMS>
			<BUFFERS>
				<BUFFER NAME="end item">
					<CONSUMING NAME="delivery end item"/>
					<PRODUCING NAME="make end item"/>
					<ITEM NAME="end item"/>
				</BUFFER>
			</BUFFERS>
			<RESOURCES>
				<RESOURCE NAME="Resource">
					<MAXIMUM NAME="Capacity" xsi:type="CALENDAR_FLOAT">
						<BUCKETS>
							<BUCKET START="2005-01-01T00:00:01">
								<VALUE>1</VALUE>
							</BUCKET>
						</BUCKETS>
					</MAXIMUM>
					<LOADS>
						<LOAD>
							<OPERATION NAME="make end item" />
						</LOAD>
					</LOADS>
				</RESOURCE>
			</RESOURCES>
			<FLOWS>
				<FLOW xsi:type="FLOW_START">
					<OPERATION NAME="delivery end item"/>
					<BUFFER NAME="end item"/>
					<QUANTITY>-1</QUANTITY>
				</FLOW>
				<FLOW xsi:type="FLOW_END">
					<OPERATION NAME="make end item"/>
					<BUFFER NAME="end item"/>
					<QUANTITY>1</QUANTITY>
				</FLOW>
			</FLOWS>
			<DEMANDS>
				<DEMAND NAME="order 1">
					<QUANTITY>10</QUANTITY>
					<DUE>2005-01-04T09:00:00</DUE>
					<PRIORITY>1</PRIORITY>
					<ITEM NAME="end item"/>
					<POLICY>PLANLATE</POLICY>
				</DEMAND>
			</DEMANDS>
		</PLAN>', true, false);
	print " OK\n";

	print "Adding an item:\n";
	frepple::readXMLData('<?xml version="1.0" encoding="UTF-8" ?>
		<PLAN xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
			<ITEMS>
				<ITEM NAME="New Item"/>
			</ITEMS>
		</PLAN>', true, false);
	print " OK\n";

	print "Saving frepple model to a string:\n";
	# @todo try this out for large strings. Memory consumption?
	print frepple::saveString();
	print " OK\n";

	print "Saving frepple model to a file:\n";
	frepple::saveFile("turbo.perl.xml");
	print " OK\n";

	print "Passing invalid XML data to frepple:\n";
	frepple::readXMLData('<?xml version="1.0" encoding="UTF-8" ?>
		<PLAN xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
			<XXDESCRIPTION>dummy</DESCRIPTION>
		</PLAN> ', true, false);
	print " OK\n";

	print "End of frepple commands\n";
};
if ($@) {
	# Processing the execeptions thrown by the above commands
	print "Exception caught: $@\n";
} 
else {
	print "All commands passed without error\n";
}

print "Exiting...\n";
