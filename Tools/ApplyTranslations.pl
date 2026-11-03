use strict;
use warnings;
use utf8;
use JSON::PP;
use Encode qw(decode encode);
binmode(STDOUT, ':encoding(UTF-8)');
binmode(STDERR, ':encoding(UTF-8)');

# usage: perl ApplyTranslations.pl <project root>  (called from ToolsPDATELOCALIZATION.BAT)
my $root = $ARGV[0] or die "root";
my $dir  = "$root/Content/Localization/Game";
my $tsv  = "$root/Config/Localization/Game_Translations.tsv";

# 번역표 읽기 (ko \t en \t ja)
my %table;
open(my $t, "<:raw", $tsv) or die "open $tsv: $!";
while (my $line = <$t>) {
	$line =~ s/\r?\n$//;
	next if $line eq '';
	my @c = split(/\t/, decode('UTF-8', $line), -1);
	die "열 수 오류: $line" if @c != 3;
	$table{ $c[0] } = { en => $c[1], ja => $c[2] };
}
close $t;

# 파일 읽기/쓰기. 원래 인코딩(UTF-16LE BOM 또는 UTF-8)을 유지
sub read_text {
	my ($path) = @_;
	open(my $f, "<:raw", $path) or die "open $path: $!";
	local $/; my $raw = <$f>; close $f;
	if ($raw =~ /^\xFF\xFE/)      { return (decode('UTF-16LE', substr($raw, 2)), 'utf16'); }
	elsif ($raw =~ /^\xEF\xBB\xBF/) { return (decode('UTF-8', substr($raw, 3)), 'utf8bom'); }
	return (decode('UTF-8', $raw), 'utf8');
}
sub write_text {
	my ($path, $text, $enc) = @_;
	open(my $f, ">:raw", $path) or die "write $path: $!";
	if ($enc eq 'utf16')      { print $f "\xFF\xFE", encode('UTF-16LE', $text); }
	elsif ($enc eq 'utf8bom') { print $f "\xEF\xBB\xBF", encode('UTF-8', $text); }
	else                      { print $f encode('UTF-8', $text); }
	close $f;
}

my %missing;
for my $culture (qw(ko en ja)) {
	my $path = "$dir/$culture/Game.archive";
	my ($text, $enc) = read_text($path);
	my $j = JSON::PP->new->decode($text);

	my ($filled, $total) = (0, 0);
	my $walk; $walk = sub {
		my ($node) = @_;
		$walk->($_) for @{ $node->{Subnamespaces} || [] };
		for my $c (@{ $node->{Children} || [] }) {
			$total++;
			my $src = $c->{Source}{Text};
			my $tr;
			if ($culture eq 'ko') { $tr = $src; }
			elsif (exists $table{$src}) { $tr = $table{$src}{$culture}; }
			if (defined $tr) { $c->{Translation} = { Text => $tr }; $filled++; }
			else { $missing{$src} = 1; }
		}
	};
	$walk->($j);
	write_text($path, JSON::PP->new->utf8(0)->pretty->canonical->encode($j), $enc);
	print "$culture: $filled / $total\n";
}
if (%missing) { print "번역표에 없는 문구:\n"; print "  [$_]\n" for sort keys %missing; }
else { print "누락 없음\n"; }
