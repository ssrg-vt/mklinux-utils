#!/usr/bin/perl -w

use strict;
use CGI;

my($cgi) = new CGI;

print $cgi->header;
my($color) = "blue";


print $cgi->start_html(	-head => $cgi->meta(
					{-http_equiv=>'REFRESH',-content=>'2'}
				),
			-title => "Popcorn status",
                       -BGCOLOR => $color);


my $info = qx("htdocs/tunnellino.sh");
print $cgi->h1("$info\n");

print $cgi->end_html;
