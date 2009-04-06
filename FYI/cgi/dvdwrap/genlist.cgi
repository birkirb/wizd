#!/usr/local/bin/perl


$buffer = $ENV{'QUERY_STRING'};

@pairs = split(/&/, $buffer);
foreach $pair (@pairs) {
	($name, $value) = split(/=/, $pair);

	$value =~ tr/+/ /;
	$value =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;

	$FORM{$name} = $value;
}

$title = $FORM{'title'};
$chapters = $FORM{'chapters'};
$cgipath = $FORM{'cgipath'};

print "Content-type: text/html\n";
print "\n";

for ($i=1; $i <= $chapters; $i++) {
	print "Chapter $i|0|0|$cgipath?title=$title&chapter=$i&file=dummy.mpg|\n";
}
