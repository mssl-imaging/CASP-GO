use File::Copy;

my ($progname) = ($0 =~ m#([^/]+)$#); 

$email = "ahowington\@usgs.gov";

$isisversion = "isis3.4.5";

my $usage = "

**************************************************************************
**** NOTE: $progname runs under isis version: $isisversion  ****
**************************************************************************

Command:  $progname fromlist

Where:
       fromlist = Ascii file containing a list of input MRO CTX EDR image
       filenames with extensions (.IMG). Each filename must be on a
       separate line in the list. If the input files reside in a directory
       other than the present-working-directory, you must also include the
       path to the images.

       NOTE: Download MRO CTX EDR images from PDS Imaging Node Atlas:
             http://pds-imaging.jpl.nasa.gov/search/index.jsp

Description:

       $progname performs ISIS3 processing starting with MRO CTX PDS images
       to create the images and files neeeded for Socet Set stereoprocessing.
       Specifically, $progname:

          1) runs mroctx2isis to import the PDS image into ISIS
          2) runs ctxcal to radiometrically calibrate the ISIS cube.
          3) runs ctxevenodd to remove detector striping.
          4) runs spiceinit, with attach=yes to add SPICE blobs
          5) runs socetlinescankeywords to convert SPICE data into a file of
             Socet Set USGSAstroLineScanner keywords and values (*_keywords.lis)
          6) runs campt to get the image statistics
          7) converts the ISIS cube to 8-bit and report the stretch pairs
             used for the conversion to *_STRETCH_PAIRS.lis
             center sample
          8) converts the 8-bit image to a raw file (*.raw)
          9) cleans up temporary files
         10) For USGS only, generates Socet Set <-> ISIS control net
             translation pvl files

       The output filenames are built from the 'core' name of the input
       images, with .lev1.cub, .8bit.cub, *.pvl, *_keywords.lis and *.raw
       appended.

       You will need to bring only the *.raw and *_keywords.lis files to
       your Socet Set workstation and run import_pushbroom to do the import.

       Errors encountered in the processing goes to files:
       \"mroctx2isis.err\" and \"mroctx2isis.prt\"

       Any errors with ISIS programs will cause this script to abort.

**************************************************************************
**************************************************************************
NOTICE:
       $progname runs under isis version: $isisversion
       This script is not supported by ISIS.
       If you have problems please contact the original author Annie Howington-Kraus
       at USGS: $email

**************************************************************************
**************************************************************************
";

  $| = 1;
  $GROUP = `printenv GROUP`;
  chomp ($GROUP);

  if ($GROUP eq "flagstaf")
     {
     $ISISVERSION = `printenv IsisVersion`;
     chomp ($ISISVERSION);
     $len = length($ISISVERSION);
     if ($len == 0)
        {
        print "\nISIS VERSION MUST BE ESTABLISHED FIRST...ENTER:\n";
        print "\nsetisis $isisversion\n\n";
        exit 1;
        }
     }

  $ISISROOT_bin_path = `printenv ISISROOT`;
  chomp($ISISROOT_bin_path);
  $ISISROOT_bin_path = $ISISROOT_bin_path . "/bin";

   if ($#ARGV < 0 || $#ARGV > 1)
      {
      print "$usage\n";
      exit 1;
      }

   $fromlist = $ARGV[0];


   if (-e "mroctx2isis.prt") {unlink("mroctx2isis.prt");}
   if (-e "mroctx2isis.err") {unlink("mroctx2isis.err");}


   $log = "mroctx2isis.err";
   open (LOG,">$log") or die "\n Cannot open $log\n";


   if (!(-e $fromlist))
      {
      print "*** ERROR *** Input list file does not exist: $fromlist\n";
      print "mroctx2isis.pl will terminate\n";
      exit 1;
      }


   if (-e "temp0101010") {unlink "temp0101010";}
   $cmd = "cp $fromlist temp0101010";
   system ($cmd);
   unlink $fromlist;
   $cmd = "cat temp0101010 | sed s/\*// > $fromlist";
   system ($cmd);
   unlink "temp0101010";

  open(LST,"<$fromlist");
  while ($input=<LST>)
     {
     chomp($input);
     if (!(-e $input))
       {
       print "*** ERROR *** Input image does not exist: $input\n";
       print "mroctx2isis.pl will terminate\n";
       exit 1;
       }

     $firstdot = index($input,".");
     $core_name = substr($input,0,$firstdot);

     $lev0Cube = $core_name . ".cub";
     $cmd = "mroctx2isis from=$input to=$lev0Cube";
     system($cmd) == 0 || ReportErrAndDie ("mroctx2isis failed on command:\n$cmd");

     $calCube = $core_name . ".cal.cub";
     $cmd = "ctxcal from=$lev0Cube to=$calCube";
     system($cmd) == 0 || ReportErrAndDie ("ctxcal failed on command:\n$cmd");

     $lev1Cube = $core_name . ".lev1.cub";
     $cmd = "ctxevenodd from=$calCube to=$lev1Cube";
     system($cmd) == 0 ||  ReportErrAndDie ("ctxevenodd failed on command:\n$cmd");

     $cmd = "spiceinit from=$lev1Cube attach=yes";
     system($cmd) == 0 || ReportErrAndDie ("spiceinit failed on command:\n$cmd");

     $cmd = "rm $calCube $lev0Cube";
     system($cmd) == 0 || ReportErrAndDie ("spiceinit failed on command:\n$cmd");

     unlink($lev0Cube);
     unlink($calCube);

  }

  close LST;

   rename ("print.prt","mroctx2isis.prt");


   if ($GROUP eq "flagstaf")
   {
      if (-e "temp_cubelist") {unlink temp_cubelist;}
      open (TEMP, ">temp_cubelist");

      open(LST,"<$fromlist");
      while ($input=<LST>)
      {
         chomp($input);
         $firstdot = index($input,".");
         $core_name = substr($input,0,$firstdot);
         $cube = $core_name . ".lev1.cub";
         print TEMP "$cube\n";
      }

      close TEMP;
      close LST;

      $cmd = "mroctxTransPvl.pl temp_cubelist";
      system($cmd) == 0 || ReportErrAndDie("mroctxTransPvl.pl failed.");

      unlink temp_cubelist;
   }

   close (LOG);

   @lines = `cat $log`;
   if (scalar(@lines) > 0)
      {
      print "\n*** Errors detected in processing ***\n\n";
      print @lines;
      print "\n";
      print "\n*** See mroctx2isis.prt for details ***\n\n";
      }
   else
      {
      unlink ($log);
      }

   exit;

sub ReportErrAndDie
    {
    my $ERROR=shift;

    print "$ERROR\n";
    print "See mroctx2isis.prt for details\n";
    print "mroctx2isis.pl aborted\n";

    rename ("print.prt","mroctx2isis.prt");

    print LOG "$ERROR\n";
    close(LOG);
    exit 1;
    }
