#!/usr/bin/perl -w

use Tk;

my $pid = $$;
my $outfil = "/tmp/paste_tmp.$pid";

sub give_up {
    my $msg = shift;

    my $errw = MainWindow->new;
    $errw->title("i've died...");
    $errw->Label(-text => $msg)->pack();
    $errw->Button(-text => "bye.", 
		  -command => sub { exit } )->pack();
    MainLoop;
}

sub try_again {

    my $msg = shift;

    my $errw = MainWindow->new;
    $errw->title("oops!");
    $errw->Label(-text => $msg)->pack();
    $errw->Button(-text => "bye.", 
		  -command => sub { $errw->destroy } )->pack();
    MainLoop;
}

sub do_palmdoc {
    my $txtw = shift;
    my $docnamew = shift;

    my $docname = $docnamew->get();
    
    if ($docname eq "") {
	try_again("no doc name specified");
	return;
    }

    my $t = $txtw->get("1.0","end");
    
    open(OUTFIL,">$outfil") 
	or give_up("unable to open $outfil for writing: $!");
    
    print OUTFIL "$t";
    close OUTFIL;

    my $syscmd = "/usr/bin/txt2pdbdoc \"$docname\" $outfil $outfil.pdb";
    system("$syscmd");
    $syscmd = "/usr/bin/pilot-xfer -i $outfil.pdb";
    system("$syscmd");
    $syscmd = "rm $outfil $outfil.pdb";
    system("$syscmd");
}

sub do_print {
    my $txtw = shift;
    my $docnamew = shift;

    my $docname = $docnamew->get();
    if ($docname eq "") {
	try_again("no doc title specified");
	return;
    }

    $docname =~ s/ /_/;
    $outfil = "/tmp/$docname";

    my $t = $txtw->get("1.0","end");
    
    open(OUTFIL,">$outfil") 
	or give_up("unable to open $outfil for writing: $!");
    
    print OUTFIL "$t";
    close OUTFIL;

    my $syscmd = "/usr/local/bin/a2psb $outfil | /usr/bin/lpr";
    system("$syscmd");
    $syscmd = "rm $outfil";
    system("$syscmd");
}


sub do_clr {
    my $txtw = shift;
    $txtw->delete("1.0","end");
}


#### MAIN

my $mw = MainWindow->new;

$mw->title("paster");


#my $txtf = $mw->Frame(-height => 10 )->pack;

my $txtf = $mw->Frame;
$txtf->pack();

my $txt = $txtf->Scrolled("Text", -scrollbars => 'e' );
$txt->pack(-fill => 'y');

my $docname = $mw->Entry();

$mw->Button(-text => "print", 
	    -command => sub { do_print($txt,$docname); } 
	    )->pack(-side => "left");

$mw->Button(-text => "to palm doc", 
	    -command => sub { do_palmdoc($txt,$docname); } 
	    )->pack(-side => "left");

$docname->pack(-side => "left");

$mw->Button(-text => "done", 
	    -command => sub { exit } 
	    )->pack(-side => "right");


$mw->Button(-text => "clear", 
	    -command => sub { do_clr($txt); } 
	    )->pack(-side => "right");




MainLoop;

