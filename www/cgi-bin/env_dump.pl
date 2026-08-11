#!/usr/bin/perl
use strict;
use warnings;

my $body = "";
my $len = $ENV{"CONTENT_LENGTH"} || 0;
if ($len > 0) {
    read(STDIN, $body, $len);
}

print "Content-Type: text/plain\r\n\r\n";
print "=== ENV DUMP (perl) ===\n";
foreach my $key (sort keys %ENV) {
    print "$key=$ENV{$key}\n";
}
print "\n=== BODY ===\n";
print "$body\n";
