#!/usr/local/bin/perl

$play_title = "./dvd_playtitle";
$dvd_device = "/dev/dvd";

$buffer = $ENV{'QUERY_STRING'};

@pairs = split(/&/, $buffer);
foreach $pair (@pairs) {
	($name, $value) = split(/=/, $pair);

	$value =~ tr/+/ /;
	$value =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;

	$FORM{$name} = $value;
}

$title = $FORM{'title'};
$chapter = $FORM{'chapter'};
$angle = $FORM{'angle'};

if ($title eq "") {
	$title = "1";
}

if ($chapter eq "") {
	$chapter = "1";
}

if ($angle eq "") {
	$angle = "1";
}

$offset = $FORM{'offset'};
if ($offset eq "") {
	$range = $ENV{'HTTP_RANGE'};
	if ($range =~ /^bytes=([0-9]+)-([0-9]*)$/) {
		$offset = $1;
		$offset_end = $2;
	} else {
		$offset = 0;
		$offset_end = 0;
	}
}

$command = "$play_title $dvd_device $title $chapter $angle $offset $offset_end 2>/dev/null";

system($command);
#print "Content-type: text/plain\n\n";
#print "$command\n";
