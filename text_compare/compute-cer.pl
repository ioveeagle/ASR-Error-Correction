#!/usr/bin/perl -CSDL
###
# This perl script calculate character error rate (CER) of Mandarin speech recognition
#
# USAGE: perl [this_script] REF HYP [LV]
#        REF : reference string
#        HYP : hypothesized string
#	 LV  : linguistic accuracy level ("word" or "syllable". If left empty, "character" used)
#
#        $BYTELEN controls the number of bytes per Chinese character
#        @costError controls deletion, insertion, and substitution penalties
#
# OUTPUT:
#         
# NOTE:
#       Default output is character accuray rate. To enable word accuracy rate of syllable accuracy rate, 
#       please set LV to "word" or "syllable". i.e.,
#
#       Evaluate syllable accuracy:
#         > perl [this_file] REF HYP 
#       Evaluate word accuracy:
#         > perl [this_file] REF HYP word 
#       Evaluate syllable accuracy
#         > perl [this_file] REF HYP syllable
#use utf8;
use Encode;
#binmode(STDIN, ':utf8');
#binmode(STDOUT, ':utf8');
#binmode(STDERR, ':utf8');

$BYTELEN = 1; ### BIG-5:=2   UTF-8:=3
@costError = (1, 1, 1); ### (del, ins, sub) :: NIST (3,3,4); HTK (7,7,10)

$SHOW_SCORE = 1; ### show basic scoring results
$SHOW_MATRIX = 0; ### show the edit distance matrix
$errorAnalysis = 1;

### read syllable table
if($ARGV[2] eq "syllable"){
	#open(FID, "/home/banco/HUB4NE/searchNet-syllable/SOURCE_DIC") or die ("Cannot open SOURCE_DIC\n");
	open(FID, $ARGV[3]) or die ("Cannot open SOURCE_DIC file $ARGV[3]\n");
	@sylFile = <FID>;
	foreach (@sylFile){
		chomp($_);
		@tmp = split(" ", $_, 2);
		$sylTable{$tmp[0]} = $tmp[1];
	}
	close(FID);
}

### read reference
open(FID, $ARGV[0]);
@refFile = <FID>;
close(FID);

### read hypothesis
open(FID, $ARGV[1]);
@hypFile = <FID>;
close(FID);

foreach $refLine (@refFile){
  chomp($refLine);
  @tmp = split(/\t/, $refLine);
  $tmp[1] =~ s/[\(|\)]//g;
  $refs{$tmp[1]} = $tmp[0];
  #printf "File %s contains %s\n", $tmp[1], $tmp[0];
}

foreach $hypLine (@hypFile){
  chomp($hypLine);
  @tmp = split(/\t/, $hypLine);
  @tmp2 = split(/\)/, $tmp[1]);
  $tmp2[0] =~ s/\(//;
  $tmp2[0] =~ s/\s+/ /g; 
  if( $tmp[0] ne "NULL" ){
    push @{$hyps{$tmp2[0]}}, $tmp[0];
  }
}

#showRefHypStrings(%refs, %hyps);

foreach $kval (sort keys %hyps){
  if(exists($refs{$kval})){ 
    #print $totalSent,"\n";    
    $message="";
    @messageIDS=();

    ### Reference
    @tmp = split(/ /, $refs{$kval});
    if( $ARGV[2] eq "word" ){
      @refToken = @tmp; ### use word directly 
    }else{
      @refToken = getTokenFromArray(@tmp, $BYTELEN); ### split word into characters
      if( $ARGV[2] eq "syllable" ){
        @refToken = getSyllableID(@refToken); ### map character to syllable
      }
    }
    
    if( $SHOW_SCORE ){
      $message.="FILE: $kval\n";
      #print "FILE: ".$kval."\n";
      #print "REF: ".join(" ", @refToken), "\n"; 
      #exit;
    }
    
    ### Hypothesis
    foreach $hypStr (@{$hyps{$kval}}){
      @tmp = split(/ /, $hypStr);

      if( $ARGV[2] eq "word" ){
        @hypToken = @tmp; ### use word directly
      }else{
        @hypToken = getTokenFromArray(@tmp, $BYTELEN); ### split word into characters
        if( $ARGV[2] eq "syllable" ){  @hypToken = getSyllableID(@hypToken); }### map character to syllable     
      }


      ### deletion cost
      for($p=0; $p<=$#refToken+1; $p++){ 
        $costMatrix[0][$p] = $p*$costError[0];
        $costDecision[0][$p] = "D";
      }
      ### insertion cost
      for($p=0; $p<=$#hypToken+1; $p++){ 
        $costMatrix[$p][0] = $p*$costError[1];
        $costDecision[$p][0] = "I";
      }
      
      for($p=1; $p<=$#hypToken+1; $p++){
        for($q=1; $q<=$#refToken+1; $q++){
          #printf("%d %d: %s %s\t%s %s\n", $p, $q, $refToken[$q-1], $hypToken[$p-1], pack("h4", $refToken[$q-1]), pack("h4", $refToken[$p-1]));
          if( $refToken[$q-1] ne $hypToken[$p-1] ){
          ### del, ins, sub
            push(@compareArray, ($costMatrix[$p][$q-1]+$costError[0], $costMatrix[$p-1][$q]+$costError[1], $costMatrix[$p-1][$q-1]+$costError[2]) );
            @minArray = getMinFromArray(@compareArray);
            
            ### $minArray[0]: cost value
            ### $minArray[1]: type of errors 
            $costMatrix[$p][$q] = $minArray[0];
            if( $minArray[1]==0 ){
              $costDecision[$p][$q] = "D";
            }elsif( $minArray[1]==1 ){
              $costDecision[$p][$q] = "I";
            }else{
              $costDecision[$p][$q] = "S";
            }
            @compareArray = ();

          }else{
            $costMatrix[$p][$q] = $costMatrix[$p-1][$q-1];
            $costDecision[$p][$q] = "M";
          }
        } ### END_LOOP refToken
      } ### END_LOOP hypToken
     
      @DecisionPath = getBestPath($#hypToken+1, $#refToken+1, @costDecision);
      #@RecogResult = getRecogResult(@DecisionPath, $#refToken);
      $RecogResultStr = getRecogResult(@DecisionPath, $#refToken);
      @RecogResult = getResultStat($RecogResultStr);

      getAlignedTranscription(@DecisionPath, @refToken, @hypToken);


      $totalC += $RecogResult[0];
      $totalM += $RecogResult[1];
      $totalD += $RecogResult[2];   
      $totalI += $RecogResult[3];   
      $totalS += $RecogResult[4];   
      $totalCorr += $RecogResult[5];
      $totalAcc += $RecogResult[6];
      $totalSent++;

      if( ($RecogResult[0] == $RecogResult[1])&&($RecogResult[2]==0)&&($RecogResult[3]==0)&&($RecogResult[4]==0)){
        $sentPerfect++;
      }


      if( $SHOW_SCORE ){
        print $message;
        print "Edit_Distance: ", $costMatrix[$#hypToken+1][$#refToken+1], "\n";

        if( $RecogResult[0] <= 0 ){ $factor = 0; }else{ $factor = 1/$RecogResult[0]; }
        printf("Utterance:\tCorr:%.2f Acc:%.2f\t", 100*($RecogResult[0]-$RecogResult[2]-$RecogResult[4])*$factor, 100*( $RecogResult[0]-$RecogResult[2]-$RecogResult[3]-$RecogResult[4])*$factor );
        printf("C:%d M:%d D:%d I:%d S:%d\n", $RecogResult[0], $RecogResult[1], $RecogResult[2], $RecogResult[3], $RecogResult[4]);

        print reverse @messageIDS;
        if( $SHOW_MATRIX ){
            print join(">", @DecisionPath), "\n";
        }
        print "\n";
      }
      @costMatrix = ();
      @costDecision = ();

      last; ### only evaluate Top-1
    } ### END_LOOP each hypothesis string

  }

}

printf("\n#### Summary of %d sentences\n", $totalSent);
printf("Sentence Accuracy: %.2f (%d/%d)\n", 100*$sentPerfect/$totalSent, $sentPerfect, $totalSent);
printf("Character-level:\tCorr:%.2f Acc:%.2f\t", 100*($totalC-$totalD-$totalS)/$totalC, 100*($totalC-$totalD-$totalI-$totalS)/$totalC);
printf("C:%d M:%d D:%d I:%d S:%d\n", $totalC, $totalM, $totalD, $totalI, $totalS);

#################################
#################################
#
sub getSyllableID
{
  my @tmpSylId;

  foreach (@_){
    #print $_, " => ", $sylTable{$_}, "\n";
    if( exists($sylTable{$_}) ){
      push(@tmpSylId, $sylTable{$_});  
    }else{
      #print "Cannot find syllable ID for ", $_, " => ", $sylTable{$_}, "\n";
      push(@tmpSylId, "UNK");
    }
  }
  return @tmpSylId;
}

sub getResultStat
{
  my @rStat;
  @tmp = split(/ /, $RecogResultStr);
  $rStat[0] = $tmp[0]; ### total char.
  $rStat[1] = substr($tmp[1], 2); ### M
  $rStat[2] = substr($tmp[2], 2); ### D
  $rStat[3] = substr($tmp[3], 2); ### I
  $rStat[4] = substr($tmp[4], 2); ### S
  $rStat[5] = substr($tmp[5], 5); ### Corr
  $rStat[6] = substr($tmp[6], 4); ### Acc  

  return @rStat;
}

sub getRecogResult
{
  my @recogArray;
  foreach (@DecisionPath){
    if( $_ eq "M" ){
      $recogArray[0]+=1;
    }elsif( $_ eq "D" ){
      $recogArray[1]+=1;     
    }elsif( $_ eq "I" ){
      $recogArray[2]+=1;
    }else{
      $recogArray[3]+=1;
    }
  }
  #$showStr = sprintf("%d M:%d D:%d I:%d S:%d %.2f", $#DecisionPath+1, $recogArray[0], $recogArray[1], $recogArray[2], $recogArray[3], 100*$recogArray[0]/($#DecisionPath+1));
  
  if( $#refToken < 0 ){ $factor = 0; }else{ $factor = 1/($#refToken+1); }
  $showStr = sprintf("%d M:%d D:%d I:%d S:%d Corr:%.2f Acc:%.2f", $#refToken+1, $recogArray[0], $recogArray[1], $recogArray[2], $recogArray[3], 100*($#refToken+1-$recogArray[1]-$recogArray[3])*$factor,100*($#refToken+1-$recogArray[1]-$recogArray[2]-$recogArray[3])*$factor );
  
  #print $showStr, "\n";
  #return @recogArray;
  return $showStr;
}

sub getBestPath
{
  my @DM;
  my @DP;
  my ($p, $q);
  my $ENDCOND = 1;

  $p = shift; ### hypothesized string length
  $q = shift; ### reference string length
  $pb = $p; $qb = $q;
  @DM = @costDecision; 
 
  #print "len(ref):$qb\t", join(" ", @refToken), "\n";
  #print "len(hyp):$pb\t", join(" ", @hypToken), "\n";
 
  while($ENDCOND){
    if( ($p==0) and ($q==0) ){
      $ENDCOND = 0;

      ### turn the following loop to TRUE shows the string alignment matrix
      if($SHOW_MATRIX){
      for($p=0; $p<=$pb; $p++){
        for($q=0; $q<=$qb; $q++){
          print " ", $DM[$p][$q];
        }
        print "\n";
      }
      }

      return reverse @DP;
      #last;
    }

    push(@DP, $DM[$p][$q]);
    if( ($DM[$p][$q] eq "M") or ($DM[$p][$q] eq "S") ){

      if($errorAnalysis){
      ### analyze substitution errors; the first token is from ref., and the 2nd is from hyp.
        if($DM[$p][$q] eq "S"){
          push @messageIDS, sprintf("SUB: %s -> %s\n", $refToken[$q-1], $hypToken[$p-1]);
        }elsif($DM[$p][$q] eq "M"){
          push @messageIDS, sprintf("MCH: %s -> %s\n", $refToken[$q-1], $hypToken[$p-1]);
	}
      }


      $p=$p-1;
      $q=$q-1; 
    }elsif( $DM[$p][$q] eq "D" ){
      push @messageIDS, sprintf "DEL: %s ->\n", $refToken[$q-1];
      #push(@messageIDS, "DEL: $refToken[$q-1] ->\n";
      $q=$q-1;
    }elsif( $DM[$p][$q] eq "I" ){
      push @messageIDS, sprintf "INS: -> %s\n", $hypToken[$p-1];
      #push(@messageIDS, "INS: -> $hypToken[$p-1]\n";
      $p=$p-1;
    }
  }

  #return reverse @DP;
}

sub getMinFromArray
{
  my ($p, $minIdx, $minVal);
  my @retArray;
  my @tmp;
  @tmp = @_;

  $minIdx = 0;
  $minVal = $tmp[0];
  for($p=1; $p<=$#tmp; $p++){
    if( $tmp[$p] < $minVal ){
      $minVal = $tmp[$p];
      $minIdx = $p;
    }
  }
  push(@retArray, ($minVal, $minIdx));
  return @retArray;
}
sub getTokenFromArray
{
  my @TA;
  my $p;
  my $token;
  my $tlen;
  my $tlen_byte;

  foreach $token (@tmp){
      $tlen = length($token);
      $tlen_byte = length(Encode::encode('UTF-8', $token));

      if( $tlen == $tlen_byte ){
        push(@TA, $token);
      }else{
        push(@TA, split("", $token));
        #for($p=0; $p<length($token); $p+=$BYTELEN){
        #  push(@TA, substr($token, $p, $BYTELEN));   
        #}
      }  
  }
  return @TA;
}

sub showRefHypStrings
{
foreach $kval (keys %hyps){
  $n = $#{$hyps{$kval}};
  print $kval, ":\n";
  print $refs{$kval}, "\n";
  for($i=0; $i<=$n; $i++){
    printf("\t%s\n", $hyps{$kval}[$i]);
  }
}
}

sub getAlignedTranscription
{
  my $i;
  my @r;
  my @h;
  my $ridx;
  my $hidx;
  my $maxlen;
  my ($rlen, $rlen_byte, $hlen, $hlen_byte);
  my ($rtype, $htype); ## '0': english, '1': non-english

  $ridx=0;
  $hidx=0;
  for($i=0; $i<=$#DecisionPath; $i++){
    $rlen_type = $hlen_type = $rlen = $hlen =0;
    $rlen_byte = length(Encode::encode('UTF-8', $refToken[$ridx]));
    $hlen_byte = length(Encode::encode('UTF-8', $hypToken[$hidx]));
    $rlen = length($refToken[$ridx]);
    $hlen = length($hypToken[$hidx]);
    
    if($rlen_byte>$rlen){$rtype=1;}else{$rtype=0;}
    if($hlen_byte>$hlen){$htype=1;}else{$htype=0;}
    

    #printf "%s %s %s %d %d\n", $DecisionPath[$i], $refToken[$ridx], $hypToken[$hidx], $rlen, $hlen;
    if( $DecisionPath[$i] eq "M" ){
      push(@r, $refToken[$ridx]);
      push(@h, $hypToken[$hidx]);
      $ridx++;
      $hidx++;
    }elsif( $DecisionPath[$i] eq "S" ){
      if( ($rtype == 1) and ($htype == 0) ){
        if($rlen*2>$hlen){
          push(@r, $refToken[$ridx]);
          push(@h, " "x($rlen*2-$hlen).$hypToken[$hidx]);
        }else{
          push(@h, $hypToken[$hidx]);
          push(@r, " "x($hlen-2*$rlen).$refToken[$ridx]);
        }
      }elsif( ($rtype == 0) and ($htype == 1) ){
        if($rlen>$hlen*2){
          push(@r, $refToken[$ridx]);
          push(@h, " "x($rlen-2*$hlen).$hypToken[$hidx]);
        }else{
          push(@h, $hypToken[$hidx]);
          push(@r, " "x(2*$hlen-$rlen).$refToken[$ridx]);
        }
      }else{
        if($rlen>$hlen){
          push(@r, $refToken[$ridx]);
          if($htype==0){
            push(@h, " "x($rlen-$hlen).$hypToken[$hidx]);
          }else{
            push(@h, " "x(2*$rlen-2*$hlen).$hypToken[$hidx]);
          }
        }else{
          push(@h, $hypToken[$hidx]);
          if($rtype==0){
            push(@r, " "x($hlen-$rlen).$refToken[$ridx]);
          }else{
            push(@r, " "x(2*$hlen-2*$rlen).$refToken[$ridx]);
          }
        }
      }
      $ridx++;
      $hidx++;
    }elsif( $DecisionPath[$i] eq "D" ){
      if( $rtype==1 ){$maxlen=$rlen*2;}else{$maxlen=$rlen;}
      push(@r, $refToken[$ridx]);
      push(@h, " "x$maxlen);
      $ridx++;
    }else{
      if( $htype==1 ){$maxlen=$hlen*2;}else{$maxlen=$hlen;}
      push(@r, " "x$maxlen);
      push(@h, $hypToken[$hidx]);
      $hidx++;
    }
  }
  $message.="REF: ".join(" ", @r)."\n";
  $message.="HYP: ".join(" ", @h)."\n";
  #print join(" ", @r), "\n";
  #print join(" ", @h), "\n";
}
