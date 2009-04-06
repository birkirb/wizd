#!/usr/local/bin/perl

print "Content-type: text/html; charset=EUC-JP\n";
print "\n";

$buffer = $ENV{'QUERY_STRING'};

($returnpath, $options) = split(/\?/, $buffer);

@pairs = split(/&/, $options);
foreach $pair (@pairs) {
    ($name, $value) = split(/=/, $pair);

    $value =~ tr/+/ /;
    $value =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;

    $FORM{$name} = $value;
}

$maxpage = $FORM{"maxpage"};
$curpage = $FORM{"curpage"};
$sort = $FORM{"sort"};

$options =~ s/&/&amp;/g;

print <<EOF;
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">
<html>
<head>

   <title>Page Change</title>
   <meta SYABAS-COMPACT=OFF>
   <meta SYABAS-FULLSCREEN>
   <style>
       body, td, form { font-size: 24px; color: #ccffff; }
       a { text-decoration:none;}
    </style>
</head>


<body bgcolor="#0000AA" marginheight="10" marginwidth="20" leftmargin="0" topmar
gin="0" border="0" link="#ccffff" text="#ccffff" vlink="#ccffff" alink="#ccffff"
 bgcolor="#ffffff">

    <b>Page Change</b><br>
    $returnpath<BR>
    <font size="-2">options; $options</font><BR>
    <hr>
    <form method="GET" action="$returnpath">
EOF

print "        ページ<select name=\"page\">";
for ($i=1; $i<=$FORM{"maxpage"}; $i++) {
    print "<option value=$i";
    print " selected" if ($i == $FORM{"curpage"});
    print ">$i";
}
print "</select>\n";
print "    <input type=\"hidden\" name=\"sort\" value=\"$sort\">" if ($sort ne "");
print <<EOF
    <BR><input type="submit" value="変更">
    </form>
</body>
</html>
EOF

