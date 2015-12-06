#!/usr/bin/perl

use strict;
use warnings;
use Digest::SHA;
use Data::Dumper;
use File::Basename;

my($all) = 0;
my($data) = 0;
my($show) = 0;
my($clang) = 0;
my($debug) = 0;
my($silent) = 0;
my($nocache) = 1;
my($profile) = 0;
my($release) = 0;
my($verbose) = 0;
my($coverage) = 0;

# Process cmd line
# =================
my(@argv);
if(defined($ARGV[0])) {
    my($arg);
    foreach $arg (@ARGV) {
        my($remove) = 0;
        $remove = $coverage = 1 if(($arg eq 'coverage') || ($arg eq 'cov'));
        $remove = $verbose  = 1 if(($arg eq  'verbose') || ($arg eq 'vrb'));
        $remove = $nocache  = 1 if(($arg eq  'nocache') || ($arg eq 'nch'));
        $remove = $release  = 1 if(($arg eq  'release') || ($arg eq 'rel'));
        $remove = $profile  = 1 if(($arg eq  'profile') || ($arg eq 'prf'));
        $remove = $silent   = 1 if(($arg eq   'silent') || ($arg eq 'sil'));
        $remove = $clang    = 1 if(($arg eq    'clang') || ($arg eq 'clg'));
        $remove = $debug    = 1 if(($arg eq    'debug') || ($arg eq 'dbg'));
        $remove = $show     = 1 if(($arg eq     'show') || ($arg eq 'sho'));
        $remove = $all      = 1 if(($arg eq      'all') || ($arg eq 'all'));
        push(@argv, $arg) unless($remove);
    }
}

# Infer default targets from command line
# =======================================
my($headTargets) = '';
if($all) {
    $headTargets = 'allTargets';
} else {
    $headTargets .= ' cov' if($coverage);
    $headTargets .= ' rel' if($release);
    $headTargets .= ' prf' if($profile);
    $headTargets .= ' clg' if($clang);
    $headTargets .= ' dbg' if($debug);
}
if('' eq $headTargets) {
    $headTargets = 'parser tst';
}

# Batten down hatches if asked to do so
# ======================================
if($silent) {
    close(STDERR);
    close(STDOUT);
}

# Generate all valid combos of modes
# ===================================
sub genModes {
    my(@m);
    my(@s);
    my(@j);
    my($baseModeNames) = shift;
    my($shortBaseModeNames) = shift;
    my($nbBaseModes) = scalar(@{$baseModeNames});

    my($nbModes) = (1 << $nbBaseModes);
    for(my($j)=0; $j<$nbModes; ++$j) {
        my($modeName) = '';
        my($shortModeName) = '';
        my($justifiedModeName) = '';
        for(my($i)=0; $i<$nbBaseModes; ++$i) {
            if($j & (1<<$i)) {
                $modeName .= " ".$baseModeNames->[$i];
                $shortModeName .= $shortBaseModeNames->[$i];
                $justifiedModeName .= $shortBaseModeNames->[$i];
            } else {
                $justifiedModeName .= '.';
            }
        }
        $modeName =~ s/^[ \t\r\n]+//g;
        $shortModeName =~ s/ //g;
        $shortModeName =~ s/r//;

        #printf("considering mode $j, NAME=$modeName SHORT=$shortModeName\n");
        my($ok) = ($modeName =~ /dbg/) ^ ($modeName =~ /rel/);
        $ok = 0 if($modeName =~ /cov/) && ($modeName =~ /clg/);

        if($ok && 0<length($modeName)) {
            #print "$modeName $shortModeName\n";
            push(@j, $justifiedModeName);
            push(@s, $shortModeName);
            push(@m, $modeName);
        }
    }
    return (\@m, \@s, \@j);
}

my(@shortModeNames) = qw(d   r   p    c   g);
my(@modeNames) =      qw(dbg rel prf  cov clg);
#my(@shortModeNames) = qw(r   d   p    c  );
#my(@modeNames) =      qw(rel dbg prf  cov);
#my(@shortModeNames) = qw(r   d);
#my(@modeNames) =      qw(rel dbg);
my($comboModes, $shortComboModes, $justifiedComboModes) = genModes(\@modeNames, \@shortModeNames);

# Base flags
# ===========
my($cc) = "gcc";
my($cplus) = "g++";
my($nvcc) = "nvcc";
my($linker) = "g++ -fno-stack-protector";
my($fortran) = "gfortran++";
my(@inc) = qw(
    -I.
    -DNDEBUG
    -DBITCOIN
    -DWANT_DENSE
    -D__STDC_FORMAT_MACROS=1
);

#-DCLAM
#-DPAYCON
#-DBITCOIN
#-DDARKCOIN
#-DJUMBUCKS
#-DLITECOIN
#-DPEERCOIN
#-DTESTNET3
#-DFEDORACOIN
#-DMYRIADCOIN
#-DUNOBTANIUM
#-DPROTOSHARES


# Libraries
# =========
my(@lib) = qw(

    cb/sql.cpp
    cb/help.cpp
    cb/taint.cpp
    cb/dumpTX.cpp
    cb/closure.cpp
    cb/headers.cpp
    cb/rawdump.cpp
    cb/rewards.cpp
    cb/pristine.cpp
    cb/txoTypes.cpp
    cb/allBalances.cpp
    cb/simpleStats.cpp
    cb/transactions.cpp

    crypto/sha256_btc.cpp
    crypto/ripemd160_btc.cpp

    h9/jh.c
    h9/bmw.c
    h9/echo.c
    h9/simd.c
    h9/blake.c
    h9/fugue.c
    h9/hamsi.c
    h9/luffa.c
    h9/skein.c
    h9/keccak.c
    h9/groestl.c
    h9/shavite.c
    h9/cubehash.c
    h9/aes_helper.c

    scrypt/pbkdf2.cpp
    scrypt/scrypt.cpp

    util.cpp
    timer.cpp
    option.cpp
    rmd160.cpp
    sha256.cpp
    opcodes.cpp
    callback.cpp

    -lcrypto
    -ldl
);

# Clean stuff we don't need for bitcoin
# ======================================
if(join(' ', @inc) =~ /-DBITCOIN/) {
    my(@pass0) = grep ! /h9\//, @lib;
    my(@pass1) = grep ! /scrypt\//, @pass0;
    @lib = @pass1;
}

# Normal target and their sources
# ================================
my(%targets) = (
    parser => [qw(parser.cpp),           @lib],
    tst    => [qw(tst.cpp test.cpp -It), @lib]
);

# tst target is automatically generated
# =====================================
my($tt) = $targets{'tst'};
opendir(D, "t");
    while($_=readdir(D)) {
        next if(/^\.$/);
        next if(/^\.\.$/);
        if(/_t\.cpp$/) {
            unshift(@{$tt}, "t/$_");
        }
    }
closedir(D);

my(@copt) = qw(
    -O6
    -g0 
    -m64
    -Wall
    -flto
    -msse3
    -Wextra
    -Wformat
    -pedantic
    -std=c++11
    -ffast-math
    -march=native
    -fno-check-new
    -funroll-loops
    -Wno-deprecated
    -falign-loops=16
    -Wformat-security
    -fstrict-aliasing
    -Wstrict-aliasing=2
    -Wno-strict-overflow
    -Wno-unused-variable
    -Wno-variadic-macros
    -fomit-frame-pointer
    -Wno-unused-parameter
    -finline-limit=1000000
    -Wno-unused-private-field
    -Wno-unused-local-typedefs
);

no warnings 'qw';

sub editOut {
    my($ix) = 0;
    my($arr) = shift;
    my($elem) = shift;
    ++$ix until($arr->[$ix] =~ /$elem/);
    splice(@{$arr}, $ix, 1);
    return $arr;
}

# Tweak for weird platforms
# =========================
my($cpu) = `cat /proc/cpuinfo`;
if($cpu =~ /ARMv6/) {
    editOut(\@copt, '-march');
}

# Open Makefile for writing
# ==========================
my($allTargets) = '';
my($allArgs) = join(' ', @argv);
if($show) {
    open(Z, ">&=1") or die "Cannot reopen fd=0: $!";
} else {
    open(Z, "| /usr/bin/make -f - -r -k -j15 $allArgs".($silent ? ">/dev/null" : ""));
}

print Z <<"EOF";
#!/usr/bin/make

# Global flags
# =============
SHELL = /bin/bash

.PHONY:all allTargets clean

all: $headTargets
	\@echo done.

EOF

# Dump section for each mode
# ==========================
my($i) = 0;
my(%seenCmd);
for(my($modeIndex)=0; $modeIndex<scalar(@{$comboModes}); ++$modeIndex) {

    # Dump header
    # ===========
    my($modeName) = $comboModes->[$modeIndex];
    my($shortName) = $shortComboModes->[$modeIndex];
    my($justifiedName) = $justifiedComboModes->[$modeIndex];
    printf Z "# Start section \"%s\" (%s)\n", $modeName, $shortName;
    printf Z "# ===========================================\n\n";

    # Dump each target
    # ================
    my($target);
    my($modeTargets) = '';
    foreach $target (keys %targets) {

        my($sources) = $targets{$target};
        my($ext) = (0<length($shortName) ? "_$shortName" : "");
        my($targetName) = "$target$ext";
        $allTargets .= " $targetName";

        print Z "# Target $targetName\n";
        print Z "# ------------------\n";

        # Compute flags based on mode
        # ===========================
        my($inc) = join(' ', @inc);
        my($copt) = join(' ', @copt);
        if($modeName =~ /dbg/) {
            $copt =~ s/-s //;
            $copt =~ s/-g0 /-g3 /;
            $copt =~ s/-fomit-frame-pointer/ /g;
            $copt =~ s/-O[2-6]?/-O0 -fno-inline/;
            $inc =~ s/-DNDEBUG/-DDEBUG -D_DEBUG/;
            $copt =~ s/-finline-limit=1000000/-fno-inline/g;
        }

        if($modeName =~ /prf/) {
            $copt =~ s/-s //;
            $copt =~ s/-g0 /-g3 /;
            $copt =~ s/-fomit-frame-pointer /-fno-inline /g;
        }

        if($modeName =~ /cov/) {
            $copt =~ s/-s //;
            $copt =~ s/-g0 /-g3 /;
            $copt .=" -fprofile-arcs -ftest-coverage";
        }

        if($modeName =~ 'clg') {
            $cc = 'clang';
            $cplus = 'clang';
            $linker = 'clang';
            $copt =~ s/-flto//g;
            $copt =~ s/-O6/-O3/g;
            $copt =~ s/-falign-loops=16//g;
            $copt =~ s/-finline-limit=1000000//g;
            $copt =~ s/-fno-check-new/-D__extern_always_inline=""/g;
        }

        # Dump build rules for each component
        # ===================================
        my($src);
        my($aux) = '';
        my($objs) = '';
        my($libs) = '';
        my($flags) = '';
        my($md) = "-- $justifiedName --";
        my($s) = ($verbose ? '' : '@');
        my($hasCUDA) = 0;
        foreach $src (@{$sources}) {

            # Skip junk
            # =========
            $src =~ s/[ \t\r\n]+//g;
            next if(length($src)==0);

            # Record & skip libraries
            # =======================
            if($src =~ /^-l/) {
                $libs .= " $src";
                next;
            }

            # Record & skip flags
            # ===================
            if($src =~ /^-/) {
                $flags .= " $src";
                next;
            }

            # Record and skip unknown target type
            # ===================================
            my($known) =
                ($src =~ /\.cpp$/) ||
                ($src =~ /\.cu$/)  ||
                ($src =~ /\.c$/)   ||
                ($src =~ /\.f$/)
                ;
            unless($known) {
                $aux .= " $src";
                next;
            }

            # Generate a rule for a compilable object
            # =======================================
            my($base, $path) = File::Basename::fileparse($src);
            my($savedBase) = $base;
            $base =~ s/\.cpp$//;
            $base =~ s/\.cu$//;
            $base =~ s/\.c$//;
            $base =~ s/\.f$//;

            # If we're using ccache, add prefix
            # =================================
            my($c) = '';
            $c = 'ccache ' unless($nocache);

            # Compute build cmd
            # =================
            my($buildCmd) = '';
            if($src=~/\.f$/) {
                $buildCmd = "$s$c$fortran $copt -w -c $src";
            } elsif($src=~/\.c$/) {
                $buildCmd = "$s$c$cc -MD $inc $copt -w -c $src";
                $buildCmd =~ s/-std=c\+\+11//g;
            } elsif($src=~/\.cpp$/) {
                $buildCmd = "$s$c$cplus -MD $inc $copt -c $src";
            }

            # See if that build command has been seen before
            # ==============================================
            my($hashBuildCmd) = Digest::SHA::sha1_hex($buildCmd);
            my($obj) = ".objs/$hashBuildCmd.o";
            $objs .= " $obj";

            # If not, emit that
            # =================
            unless($seenCmd{$hashBuildCmd}) {

                $seenCmd{$hashBuildCmd} = 1;

                print Z "$obj : $src\n";
                print Z "\t\@mkdir -p .deps\n";
                print Z "\t\@mkdir -p .objs\n";

                my($dep) = $obj;
                $dep =~ s/\.o$/.d/;

                if($src=~/\.f$/) {
                    print Z "\t\@echo f77 $md $src\n" unless($verbose);
                    print Z "\t$buildCmd -o $obj\n";
                } elsif($src=~/\.c$/) {
                    print Z "\t\@echo 'cc ' $md $src\n" unless($verbose);
                    print Z "\t$buildCmd -o $obj\n";
                    print Z "\t\@mv $dep .deps\n";
                } elsif($src=~/\.cpp$/) {
                    print Z "\t\@echo c++ $md $src\n" unless($verbose);
                    print Z "\t$buildCmd -o $obj\n";
                    print Z "\t\@mv $dep .deps\n";
                } elsif($src=~/\.cu$/) {
                    print Z "\t\@echo ncc $md $src\n" unless($verbose);
                    print Z "\t$buildCmd -o $obj\n";
                    $hasCUDA = 1;
                }
                print Z "\n";
            }
        }

        # Generate rule to link this target
        # =================================
        my($v) = ($verbose ? '>/dev/null 2>/dev/null' : '');
        print Z "$targetName:$objs $aux\n";
        print Z "\t\@echo lnk $md $targetName $v\n";
        print Z "\t$s$linker $copt -o $targetName $objs $libs \${LIBS}\n";
        print Z "\n";

        # Add target to list for this mode
        # ================================
        $modeTargets .= " $targetName";
    }

    # Generate phony rule to build all targets for this mode
    # ======================================================
    my($tmp) = $shortName;
    if($tmp =~ /^([ \t]+)?$/) {
        $tmp = 'rel';
    }
    print Z ".PHONY: $tmp\n";
    print Z "$tmp: $modeTargets\n";
    print Z "\t\@echo $tmp is ready.\n";
    print Z "\n";
}


# Finish Makefile
# ===============
print Z <<"EOF";

allTargets: $allTargets
	\@echo all targets are ready.

clean:
	-rm -r -f *.o .objs .deps *.d $allTargets a.out* *.gcno .vs x64

realclean:clean
	-rm -r -f ~/.ccache

-include .deps/*

EOF

close(Z);

