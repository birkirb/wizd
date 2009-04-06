***************************************************************************
In beginning
***************************************************************************
As for this, it is something which remodelled wizd which is the server for MediaWiz of 685 person productions selfishly. This one does not guarantee every operation. Concerning this software, (Inc.) please do not inquire at the vertex link corporation. This software was used, assuming, that what damage was received, it does not do all guarantee. Please use entirely on self responsibility. You follow distribution condition and the like original wizd basically, but this software is not the case that it is made in order to gain with the behavior where the specific individual is unsound. For example, it is accustomed to selling this software in the old house and the auction. In addition, the prayer and the like you hold and you sell do not say together, but when that kind of commercial utilization is done, support in addition please take all responsibility entirely on that side. Even # that the person who cannot be protected uses!


The case of redistribution please leave the documents as much as possible.

readme File constitution
readme.txt          : wizd 0.12 (The original) it is the instruction manual which has belonged
readme2.txt         : wizd 0.12da It is the instruction manual which has belonged.
readme12f3_f4.txt   : wizd 0.12f4 It is the instruction manual which has belonged.
readme_gr.txt       : wizd 0.12gr It is the instruction manual which has belonged. (Perhaps)
readme_altered.txt  : wizd ???It is the instruction manual. (This file)

***************************************************************************
Thanks
***************************************************************************
 If 685 people do not draw up wizd, this remodelling the proper it does not exist. To make such a splendid software and being gratuitous and applying to the original author whom it releases, this much, it is extent appreciation.

If AX10USER was not, I did not know, concerning MediaWiz and concerning wizd and concerning AX10 probably will be. Being something due to the information of the person it does the majority related to SVI in wizd. It is the expectation which has blended also the cord/code of the person to part. In addition, the relationship of SVI such as investigation of trouble had doing especially each time the operation verification with AX10 carefully. It is chaotic appreciation.
 wizd When at remodelling edition 0.12e, gigo being densely, it took in wizd 0.12da which you find completely. Ozeki merged my wizd remodelling edition 0.12d and the VOB connected patch, so is. Thank you. Details are written on readme2.txt.

 wizd At remodelling edition 0.12g, wizd 0.12f4 was taken in completely. As for the writer of 0.12f4 943 person kana of AX300 ?? the 4. You merged selfishly. Please acknowledge.... And, thank you. Details are written on readme12f3_f4.txt.


 wizd At remodelling edition 0.12h, the resize of JPEG was taken in. It is appreciation.

 Basicblue The writer who the question thin makes the rose forcing skin (237 people), again it is appreciation.

In addition, you appreciate in all people who cooperate to building up this software.
***************************************************************************
Known problems
***************************************************************************
·http_passwd: The occasion where you use, it cannot handle with the certification in order to use as Proxy and the certification ahead it is access with Proxy well.

·HTTP Realm of certification "dummy"....
·With internal mounting of the play list, near plw being able to standardize it is not still. (With, once, file several restrictions being gone, ? schedule)

***************************************************************************
Modification past record
***************************************************************************
? wizd 0.12h RC3 (The ? is does, merging of gt edition still is... the ?and collapse feeling...)
·JPEG Attendant upon resize correspondence substantial revision of internal mounting of the skin.
  - SETCONF It rewrites, wizd_skin. * makes. Variety.
  - When there is no bug ? ?, or notes and observes it is delightful, is.
  - So, while furthermore aiming for large remodelling... while being held up....
·Temporarily, with quick-hack cgi of skin/ targetting operational.
·This as for nkf of point in time, 2.04 CVS latest edition (2.0.4 2005-01-24 edition)
·addition of the place of wizd.conf which is read with) default. (With, ??? easy to do in) 
·wizd.conf flag_read_mp3_tag addition. (Experiment it tries trying to move with) 
·inetd. (Make wizd_inetd: Experiment in) 
·wizd.conf wizd_chdir addition. (For inetd edition. Experiment)
·JPEG resize modification to action=Resize.jpg. 
·When JPEG with resize, the original picture is small, small way just aspect ratio conversion. 
·Skin even with directory cgi possibly. 
·When (experiment) cache is difficult to accumulate even, when 13 seconds it does, way it starts sending, modification.
 - The timeout "13 seconds of MediaWiz (guess)" the hard cord/code it does and the ?? is.... Main point modification.

---------------------------------------------------------------------------
? wizd 0.12h RC2 Therefore (the ? is does, merging of gt edition still is... RC. Secret)

·It corresponds to the POST method of CGI with spirit
·Modifying the resize of JPEG somewhat, it takes in. Just at the time of action=Resize resize.
·It meaning that nkf which is made especially and the cause the page of libnkf being out is old so, it cancels use.
·It made nkfwrap as the substitution of libnkf, tried to be able to utilize up-to-date nkf.c that way.
·When MediaWiz does not know the encoding HTML, (example: X-sjis) In order to rewrite to the encoding MediaWiz, operation of the professional comb was changed.
·Way HEX/CAP conversion, is left to nkf, modification. (--cap-input)
·As for nkf of this point in time, 2.04 CVS latest edition (2.0.4 2004-12-01 edition)
·It corresponds to extension nuv. For MythTV it seems. (Thx tmt)
---------------------------------------------------------------------------
? wizd 0.12h RC1 Therefore (the ? is does, merging of gt edition still is... RC. Secret)
·Loading just a little delicate Basic certification. (There is no meaning from MediaWiz system? )
  Failing in User-Agent check, when certification coincides to be set, access permission.
·Correcting the bug where CGI did not move normally at the time of flag_daemon true.
  If the book it was not from HTTP/1.0, the person that it did not move, this is cause.
·To standardize internal mounting of the play list formation part, somewhat modification. -> The number of files being many, it can play back normally with AllPlay. Much.
·Way extension pass it does not do at the time of the directory, correction. (Thx it is dense - the space)
·JPEG targetting AllPlay modification. (TYPE_JPEG addition. 
·The translation table of libnkf (utf8tbl.c) it replaces.
·In order to be able to operate even with Cygwin, FILENAME_MAX was replaced to WIZD_FILENAME_MAX. Because (FILENAME_MAX is used for also the processing of URL, in trouble.... Temporarily 2048 fixing)
. ()"" Modifying the place of cutting. Tsv With in the mp3 tag the husband who becomes invalid. Being related with this, in order to keep file name and the like with euc type, internal mounting well enough modification.
·Addition. As for example in skin/basicblue/head.html. It is displaced in euc-jp or Shift_JIS according to client_language_code.
·When connection first certification is necessary the case of Proxy, rewriting URL in playlist.
·Adding FYI/tools/symlink* to one for Cygwin. (Details later description)
---------------------------------------------------------------------------
? wizd 0.12g
·But operational lack of confirmation wizd 0.12f4 was taken in source completely. Sv3 Correspondence it seems with this, is. The ? ? which it does.
·At skin, it tried to cut indicatory file name on the basis of the width of the font, (details later description)
·The information of AVI was made indication possible, (experiment. FPS, duration, WxH, vcodec and the interleave. detailed later description)  <!--WIZD_INSERT_LINE_AVI_FPS-->
  <!--WIZD_INSERT_LINE_AVI_DURATION-->
  <!--WIZD_INSERT_LINE_AVI_HVCODEC-->
  <!--WIZD_INSERT_LINE_AVI_VCODEC-->
  <!--WIZD_INSERT_LINE_AVI_HACODEC-->
  <!--WIZD_INSERT_LINE_AVI_ACODEC-->
  <!--WIZD_INSERT_LINE_AVI_IS_INTERLEAVED-->
·The case where the professional comb is used with tsv fine control
·At the time of the professional comb, it conveys to the partner Host: Being not to have port number, it corrects
·At the time of the professional comb, URL it tried also the list of SinglePlay and the like to substitute
·The behavior of uri_encode a little modification (+ of space - the %20)
·As for line there is no COLUMN and ?? which is ? ROW.... Addition.
·In addition the tag which is added
  <!--WIZD_IF_PAGE_PREV--> <!--/WIZD_IF_PAGE_PREV-->
  <!--WIZD_IF_NO_PAGE_PREV--> <!--/WIZD_IF_NO_PAGE_PREV-->
  <!--WIZD_IF_PAGE_NEXT--> <!--/WIZD_IF_PAGE_NEXT-->
  <!--WIZD_IF_NO_PAGE_NEXT--> <!--/WIZD_IF_NO_PAGE_NEXT-->
  <!--WIZD_IF_STREAM_FILES--> <!--/WIZD_IF_STREAM_FILES-->
  <!--WIZD_IF_NO_STREAM_FILES--> <!--/WIZD_IF_NO_STREAM_FILES-->
  <!--WIZD_IF_LINE_IS_ODD--> <!--/WIZD_IF_LINE_IS_ODD-->
  <!--WIZD_IF_LINE_IS_EVEN--> <!--/WIZD_IF_LINE_IS_EVEN-->
·Tune name of the ID3 tag tried the time of the sky, not to refer to ID3 tag information---------------------------------------------------------------------------
? wizd 0.12fb (Trivial modification)
·SinglePlay being functioning not to have done with SVI, and VOB it corrects
? wizd 0.12fa (patch; Trivial modification)
·Extension replacement processing, with the capital letter extension being functioning not to have done well, it corrects
---------------------------------------------------------------------------
? wizd 0.12f
·Loading the simple CGI function (only GET method...)
·At the time of File Not Found,/to Redirect it tried to do
·The number of clients was made restriction possible
·One for when in the skin, when the client is PC so is not, tag addition (in WIZD_IF_CLIENT_IS_NOT_PC and the WIZD_IF_CLIENT_IS_PC. meaning note...)
·The professional comb and the execution permission of CGI were made the respective setting possible
·In skin, tag addition for date time present directory name indication
·.tsv Due to in the hypothetical directory, it tried also the link to other than the existence file to be able to stretch
·The hypothetical directory (tsv) the skin of business was drawn up. (Line_pseudo.html)
·Shuffle function loading (? Sort=shuffle)
·When with access to the directory,/with the terminal it is not done, Location it tried to send
·action=SinglePlay Even in case of the MOVIE file it tried to apply·Whether or not PC it tried to utilize the value of user_agent_pc of wizd.conf to deciding
·Only 1 time that tried applies directory same name deletion function vis-a-vis one character string
  Example; Test/testtest1.avi old: Test/1.avi new: Test/test1.avi
·When document_root is strange, putting out warning, it ends
·From the browser../../../../.. the intention of controlling the fact that it can access /etc/passwd and the like
·The tag and variable in order to appoint onloadset to the BODY tag were added. (As for details separate paragraph)
·The tag which acquires the number of lines (WIZD_INSERT_LINE_COLUMN_NUM) it added.
·AAC (m4a) it added to wizd_param.c. (Thx 371 person @Part8)
---------------------------------------------------------------------------
? wizd 0.12e (????)
. To correspond to the continual playback of VOB with the taking in wizd 0.12da, and so on and so on.
. The occasion where the professional comb is utilized, it is possible to change User-Agent mandatorily,
. Date indication of file from file status correction time modification to data correction time
. Without the extension the file made non indicatory
. .hnl - hnl.mpg The extension you read and apply add
---------------------------------------------------------------------------
?Before wizd 0.12d (the difference with the original wizd 0.12)
·Cache function of reading buffer (beginning of thing)
·Professional comb function (html rewriting function it is attached)
·The hypothetical directory function by the tsv file (for AX10? )
·The play list formation function for PC (mplayer and for winamp? Experimental cord/code)
·MP3 ID3v2???? ????
·Correspondence to the file size indication of over 2G (correction)
·When there is SVI of the same name as the AVI file, indicating that information
·When information of the SVI file is indicated, [ ] () deletion does functioning
·It meaning that the m3u file and the plw file have done the interchangeability almost, it reads also the m3u file
·Adding RECYCLER and System Volume of Windows Information to the disregard directory·??????????????????????????????
·The play list it is not included in allplay
·When being daemon, usage condition of cache progress bar indication
·It meaning that the file size acquisition of over 2G of SVI is easy to become strange, it corrects

***************************************************************************
About the skin
***************************************************************************
  As for basicblue which is attached, some hand is added to those which are attached as one for wizd 0.12.
---------------------------------------------------------------------------
The difference from for wizd 0.12 of the basicblue skin which is attached...
·MediaWiz???????????(2003/12/24?)???????????????
*Font-size of head.html from 10px was designated as 12px. (Thx, 251 people @Part7)
*Inserting nowrap, you controlled carriage return.
*Appointment of the font of date size title it could divide.
·The operation of PgUP/PgDown was made opposite. That you think whether (there probably is a for and against both theory, but if... hate resetting, don't you think?)
·At the time of the SVI file outputting the information itself of SVI. When (using this skin, as for flag_filename_cut_parenthesis_area true is completed the male)
·Setting menu_svi_info_length_max of wizd_skin.conf. (40 letters)
·The tag mistake correction of MP3 (from wizd 0.12da)
·Making use of WIZD_IF_CLIENT_IS_PC and WIZD_IF_CLIENT_IS_NOT_PC, in case of PC it tried to adjust the background and letter with the style seat. (Wizd 0.12f)
·It tried to indicate time and the present directory etc. in lowest line. (Wizd 0.12f)
·It it linked to shuffle function in place of past record. (Wizd 0.12f)
·It tried to be able to indicate the past record inside MediaWiz. (Wizd 0.12f) 0.12f)
·You removed focusing the continual playback, false scroll function (separate paragraph) mounted. (Wizd 0.12f)
·The stop button (tvid=ESC) with, /cgi-bin/page_change.cgi is called, the sea urchin it did. If page_change.cgi of attachment is installed appropriately in the place, it tried to be able to do page modification directly with folio appointment. However perl necessity. (Wizd 0.12f)
·Adding charset= to head.html. (Wizd 0.12h)
·What it changed high stopped knowing gradually and whether the ?... orz

***************************************************************************
Concerning Cygwin (wizd 0.12h)
***************************************************************************
  Wizd operates even with Cygwin.
  When you do not use with Cygwin, please ignore.

- If the enviroment for software development of gcc and the like is inserted in compiling Cygwin environment, you may have to play with the makefile.  You will need to include cygwin1.dll to run in windows. But this will make your code below to GPL license, as for distribution whether it is not possible, being is because of sense, as for execution binary of Cygwin edition please do not distribute the cygwin dlls.

  - With symbolic link normal UNIX environment or the environment where Cygwin is installed, ln -s symbolic link can be drawn up in the command such as /mnt mnt. By the fact that this is done on the document route of wizd, being another directory and something where the file on another disk can be indicated, it does, but well enough it is difficult to do this job. (Especially, the drive letter of Windows /cygdrive/c/WINDOWS/... with what it appoints. ) There, FYI/tools/symlink.c was prepared. Make please do with Cygwin environment. First, symlink.exe and cygwin1.dll which were drawn up are put on the identical directory. When and in this symlink.exe idea contest such as CD-ROM drive and folder the drug & drop is done from Windows, the symbolic link for Cygwin is drawn up. It is the expectation which has been visible in shortcut from Windows side. When this is made to move to the document route of wizd that way, the link it can access first from wizd. However, with "compilation of shortcut" of Win standard being not to be possible, please note. - It cannot distribute the one execution binary of using, but the occasion where the binary which was drawn up with the individual is used, if there is *exe and cygwin1.dll (with, something related to JPEG inserting, if the ? DLLs for that), there is no Cygwin environment itself and the ? wizd operates normally.

***************************************************************************
Concerning the font (wizd 0.12g)
***************************************************************************
When bundled basicblue is used with MediaWiz, that rather than saying, when the normal font is used, the English letter which is indicated with the menu does not become width such as each one. When bundled basicblue is used with wizd of the time before, just puts in place the file, WW.avi indication had deteriorated. 

In addition, aiueo avi there being a margin on however much right, because at just the number of letters of file name is cut, it could not indicate any the way, in completely. Then, it tried inspecting the font information of MediaWiz under circumstance of specification in the manual operation. When this data is used, under circumstance of the specification letter it is cut off securely at position of specification. Unless under circumstance of specification, with the reason which was said does not modify the size of the font even with MediaWiz and/or makes bold, information of the font changed. Increasing, you do not know the other than of MediaWiz has used some kind of font. Then, at the defined file of skin, it tried trying to be able to set the information of the font which is used. However, please be prepared. To inspect, and setting considerably it is serious. Therefore trouble like of the time before when we would like to cut at just the number of letters, if sort of the time before you can describe, it is all right. Actually this the information which was inspected is applied to bundled basicblue because, 

 Please look at wizd_skin.conf.
So, it is explanation of the actual setting with skin.
  menu_font_metric 10
  menu_font_metric_string aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaiehlkpneggikfiehkgkkkkkkkkefkkkkrolnmllnlejlkmknlnmmmlosoonfhfimglkkkkikkefkeokkkkhjikkokkjgegka

256 letters, you can describe menu_font_metric_string. 
With example, 128 letters you have described. (Remainder is buried with 0) With the first 1st letter width of character code 0x00 is set, with the next 1 letter width of character code 0x01 is set. A means width 0, b means width 1. Z is width 26. The or more of that is unreasonable. Menu_font_metric is width of the default where width is used unspecified occasion. Please think that it is Chinese character 0.5 letter phase splitting this number. By the way, if width of Chinese character and the like is not width, this design fails. Considering, increase the half angle katakana it is. You think that well enough it becomes strange. Temporarily, being bundled to do, the sample file which is used the person who becomes matter of concern the ?. (FYI/font/*) 
***************************************************************************
Concerning AVI information (wizd 0.12g)
***************************************************************************
The information which presently can be acquired is as follows.
·Playback time (duration)
·FPS(Frames per sec)
·Video/audio codec information
·Size of picture(Width x Height)
  ·Animated picture and sound being interleaved truly, the decision result whether it is arranged of,

  These are the information which is recorded to the header of AVI basically. Like below it is substituted.
  WIZD_INSERT_LINE_AVI_FPS             -> 23.976
  WIZD_INSERT_LINE_AVI_DURATION        -> 0:23:44
  WIZD_INSERT_LINE_AVI_HVCODEC         -> DX50
  WIZD_INSERT_LINE_AVI_VCODEC          -> divx
  WIZD_INSERT_LINE_AVI_HACODEC         -> MP3
  WIZD_INSERT_LINE_AVI_ACODEC          -> null

  HVCODEC and HACODEC the AVI header (strh) are information. VCODEC and ACODEC the AVI header (strf) are the information of BITMAPINFO/WAVEFORMATEX. Perhaps the method of doing the indication of here and others side and the method of writing in the future it changes.

By the way size of animated picture (Width x Height), it enters into WIZD_INSERT_LINE_IMAGE_WIDTH and WIZD_INSERT_LINE_IMAGE_HEIGHT.

... Well, being most important here, different kind the thing is decision of interleave. Unless with the present firmware, rather than with saying, in present specification by any means, the data of the picture and sound (is plugged to the small being cut off moderately and) is arranged in the interleave, the picture is played back first and, the kind of operation where only sound is played back afterwards, is done.

There is a flag which shows whether in the header of AVI, it is interleaved once. Seems that almost no one uses, (the ' ? `) there, it tried deciding by 100 inspecting the arrangement of the data inside the movi chunk of AVI. When that 100 type of the same data (animated picture and the like) has lined up together, it is the mechanism which it decides that it is not interleaved. Because is, rarely please consent the fact that it is misjudgment fixed.

By the way like below it is substituted. WIZD_INSERT_LINE_AVI_IS_INTERLEAVED - I NI? I shows the fact that it is interleaved. NI shows the fact that it is not interleaved. That (it plays back, and so on there is no sound)? It is unclear. (The file which is broken? )
  
It is proper, but these operate when the extension AVI being basic only. When it is not AVI and when you could not read information, as for the respective tag? ? : ? ? : ? ? And so on it is substituted. (As for CODEC [ none ]) as an exception, when being AVI, VCODEC is substituted to the extension. Because, because the one is easy to know, (laughing (example; [ Mpg ] And [ wmv ])
The male be completed (? Perhaps) setting just a little it is difficult to see, but temporarily Just for your information.    line_movie.html ??????…
---------
	<td nowrap class="date" colspan=2>
		<!--WIZD_INSERT_LINE_AVI_HVCODEC-->:<!--WIZD_INSERT_LINE_AVI_VCODEC--> (<!--WIZD_INSERT_LINE_AVI_HACODEC-->:<!--WIZD_INSERT_LINE_AVI_ACODEC-->) &nbsp;[<!--WIZD_INSERT_LINE_AVI_IS_INTERLEAVED-->]&nbsp;<!--WIZD_INSERT_LINE_IMAGE_WIDTH--> x <!--WIZD_INSERT_LINE_IMAGE_HEIGHT-->
	</td>
---------

Rather than future topic (with saying, your own memo): We would like to acquire also the information of wmv and mpg. ......... Being too complicated, considerably trouble REPT (the ` ? ') no HVCODEC and VCODEC (A) adds the same time the tag where becomes indication just of one side? The information which just a little can be acquired we would like to increase

***************************************************************************
Concerning CGI (wizd 0.12f)
***************************************************************************
Well it is usual thing, but it mounted suitably. When AX10USER would like to do file execution, with thing, mounting the result CGI which, is considered it came to the point of offering. Because of this, rather than imbedding the script execution of specification, it is the expectation where general purpose rises. In other words, normal CGI it is to be the husband, it is the expectation which it is possible freely regardless. When change it operates please teach what. For the present, is only GET.... Furthermore, (with QUERY_STRING) the data which can be handled the bean jam ball is not long. Note....

Presently, cgi being not to have put out in the file list, inputting URL directly, (writing on or html) please execute the file from there. Please give execution permission to the CGI file. When (man chmod) operation system such as file deletion description/executing with CGI, please pay attention to various security. When example is listed, delete_file.cgi? At the time of file=hoge.avi delete_file.cgi? File=.. /wizd.conf and show_file.cgi? At the time of file=hoge.txt show_file.cgi? When file=../../../../../../.. /etc/passwd and the like and the like is executed, (or making a mistake? When you do) ?? to be, is. On wizd side measure it does not do, (it is not possible).

Because as for environment variable measure you think the thing, that it is it mounted, please utilize. PATH, because it is reset to standard ones, when the program which is /usr/local/bin/ and the like is used and the like, the male is completed does full pass inscription.

In addition, when the CGI program spits out the animated picture data, when it links normally, the extension becomes, cgi with does not recognize animated picture. Then, "play_mpeg.cgi? Start=100&file=dummy.mpg "the way, end mpg and so on please appoint the argument which can recognize MediaWiz. After, vod= "0" not forgetting. But, because this operation depends on the firmware of present MediaWiz, when there is modification, with this there is a possibility of stopping going well. In addition, there is also a possibility of not operating in the other type. Because that time another method is mounted, please inform. (Being to become beautiful, being not to like to do it does, "another method" but. If) (it carves with Content-type, to being good, with it does not have to be regrettable)

Utilization example
   ·Whether or not the client PC because the tag which can be distinguished was added, combining, when perusing from PC, indicating the button in the list, when you push, bit rate conversion and operation of the file
   ·Calling the DVD reading program, (example; Play_title of libdvdread) the playback of DVD

  Known problem
   ·Because it is omission, the CGI program which is called cannot finish the header with empty line, if, because it forwards to the client all data as a header, note.

***************************************************************************
Concerning the restriction of the number of clients (wizd 0.12f)
***************************************************************************
Because wizd fork () executes vis-a-vis connection request, when access is many, itself copy occurs in large quantities. Perhaps it becomes DoS state. When is little memory with such as installed environment, most ?? it is and with thinks. Especially, using the professional comb from IE, when there is a picture in large quantities in the page ahead the professional comb, because the reading occurs in parallel, it becomes such situation, probably will be. When in reality, this is done with AX10, there was the report that AX10 fell. Then, fork of the connection acceptance () it tried to be able to bet restriction on before. With AX10 it stopped falling with this, so is; -) Because the bean jam ball it does not test this, when there is trouble please teach.

***************************************************************************
Concerning the hypothetical directory (wizd 0.12f)
***************************************************************************
To wizd 0.12d tsv by with the hypothetical directory, as for the file which in that can be described, it existed and it was necessary to be the extension which can recognize wizd. But, the restriction was removed from wizd 0.12f. Because of this, link to the external page and the link to CGI can be drawn up easily. Type of the file usual Tab Separated Values (TSV) is the file of type. ---- sample
Somewhere/somefile.mpg [ TAB ] 0 [ TAB ] the name which is indicated [ carriage return ] http: //www.example.com/ [ TAB ] 0 [ TAB ] ? ?? ? - ? [ carriage return ] /-. -http: //www.example.com/ [ TAB ] 0 [ TAB ] ? ? paragraph [ carriage return ]
-------------
1st field appoints the anchor ahead linking. 
2nd field appoints TVID, but with wizd you ignore. 
3rd field appoints the name which is indicated on the picture.

If the file exists, usual processing is done on the basis of the information. Example; File size, file such as fail clean up day and time indication and the tag information reading of mp3 does not exist and the ?, the extension of the anchor is judged, usual vod addition and setting etc. of SiglePlay mode are done.

Known bug
  ·When processing of the name conversion of SVI and mp3 etc. enters, name appointment is superscribed.
  ·Being to be omission, when the anchor ahead linking is absolute pass appointment, there is a possibility of reading the information of another file. Rather than with saying, mostly full pass cannot be formed just.
  ·Ahead linking usual http: / In case of link of type, if the file is searched and as a relative appointment that exists, information of that file is indicated, (laughing After all, because the bean jam ball it does not test this, when there is trouble please teach.

***************************************************************************
Concerning onloadset and focus variable of the BODY tag (wizd 0.12f)
***************************************************************************
  This function basically is something because the false scroll is actualized on the file list. With basicblue of attachment, when the upper key is pushed with best line, when in lowest line of the front page, the lower key is pushed with lowest line, it has reached the point where it moves to the best line of the following page. (Information thx 104 person @Part8)

Because this is actualized? The variable, focus= was added. When this is appointed, in the WIZD_INSERT_ONLOADSET_FOCUS tag, onloadset= "$focus" is substituted (here as for $focus when the right-hand value of focus=) being appointed, as for that tag becomes the sky. (It is deleted)

But, when really it tries doing, being to be troubled when directly the directory is appointed it added WIZD_IF_FOCUS_IS_SPECIFIED and the WIZD_IF_FOCUS_IS_NOT_SPECIFIED tag and it tried to be able to define onloadset of default by the fact that the BODY tag is surrounded. As for details please look at basicblue.

Furthermore, being something where A, NAME START and end is appointed to only the best line and lowest line of the file list, it did, original idea, but from the point that, the following tag was added, with wizd line_* html the point that rewriting with when it is difficult it is not beautiful, from the same thing was actualized general-purpose with that. That it returns the WIZD_INSERT_LINE_COLUMN_NUM tag which is added, simply it is no line eye on the picture. Best line is 1. With attachment basicblue the fact that, name= "L" making use of this has been added to the A tag. Like this when it does, in each line name=L1? L13 (13 is attached page_line_max of wizd_skin.conf), that becomes name. With this, best line, it is possible with focus=L1 to appoint lowest line with focus=L13. And one than best line ago with onfocusload the false scroll is actualized link of the page movement which does this appointment, by the fact that it arranges, with one next A tag of lowest line.

By the way, when WIZD_INSERT_LINE_COLUMN_NUM each line_* html is appointed to the place of tvid, because on the page the file of some turn, with it can input with TVID which is said, the person that, we are liked this, when so it rewrites, is good, probably will be.
 
***************************************************************************
About cache buffer
***************************************************************************
  Transfer of the file original wizd has done mounting which differs. This part (with the professional comb) is main function of wizd remodelling edition.

In this cash buffer, 10240 bytes (10KB) it designates block as the basis and plural gathers that block and has made one cash. With buffer_size of wizd.conf, this quantity can be adjusted. For example when we assume, that 100 there is block, cash 100 * is 10KB = 1000KB. Default is 1024. In other words, 10MB it guarantees. Environment and the installed environment whose memory is little (example AX10) and the like with, please decrease.

If we would like to guarantee the same size as original wizd, please set to 13. It meaning that system is different, completely it does not become with the same operation, but generally it is the expectation which becomes same.

Presently, the block which had cash 1/4 storing rare of all the block counts, it has reached the point where playback is begun. Depending, until it is really played back, there are times when some time is required, but
  1) Buffer_size is decreased
  2) It tries designating flag_buffer_send_asap as true, (non recommendation) and the like please try doing.
When conversely, it stops midway, please try increasing buffer_size. There are also times when with decoding error it is visited in similar phenomenon depending upon the file. Because it is possible, to indicate use condition of cash in the picture, while verifying with that, you think that it is good to adjust. It indicates in, at wizd.conf please designate flag_daemon as false. In addition perhaps it is easy to know, that collectively, it makes debug_log_filename /dev/tty, sets flag_debug_log_output to true. When (the client is plural, indication deteriorates. Please note. )

This buffer, network (the professional comb) the occasion where such as NFS it reads from the slow device of /CDROM and the like, shows especially power. Because is, you can assert (you read from only local HDD) when, that high speed it is reading environment, excessively there is no meaning. When the buffer is taken too largely, local being, delay occurs in playback, but it is few to become problem, probably will be. Note) still making, increase the transfer by cash buffer of animated picture of the SVI file it is.

***************************************************************************
How to use the professional comb function
***************************************************************************
(From the URL button with remote control it is good of course even with input, is, but that compared to) prepares following kind of html, places on the root directory of wizd,
You make read to MediaWiz. Because (of course example.org is example, lower does not exist)
---
  <A HREF="/-.-http://www.example.org/sample.avi" vod="0">sample.avi</A><BR>
  <A HREF="/-.-http://www.example.org/dir/">dir list @ example.org</A><BR>
---
Like this, while by the fact that it does, wizd acquiring the data in substituting because the data is transferred to MediaWiz side, it is possible to receive the benefit of buffer cash.

With example of 2nd directory list, A HREF in HTML which is received? When link because the professional comb of wizd it rewrites with the extension, for example avi the file is included in the list, vod= "0" is attached automatically. In addition, when the access from PC (*) is, furthermore, /-. -playlist.pls? The rewriting to http is done. (As for details later description)  
In addition, with this professional comb (MediaWiz does not correspond) can use Basic certification.
---
  <A HREF="/-.-http://user:passwd@www.example.org/sample.avi" vod="0">sample.avi</A><BR>
  <A HREF="/-.-http://user:passwd@www.example.org/dir/">dir list @ example.org</A><BR>
---
Some time ago when it is example, this way please appoint. Because it means that the password is buried in html, don't you think? please pay attention to use. Because note) strict rewriting processing is not done, there are times when it does not go well depending upon the prescribed form of html and the transfer circumstance of the html file from network.
***************************************************************************
Being defeated
***************************************************************************

By the fact that job below the play list function for PC is done, even on PC animated picture/sound can be enjoyed from wizd. Note! Because) to the last it is experiment, as for the person who is not understood well please do not use.

When the access from PC (*) is, wizd forms in the menu screen which, vod= "0" is attached, as for the animated picture/audio kind of file where, way you can peruse even from PC, link /-. -playlist.pls? Http: /... With it becomes the type which is said. When it accesses this, in audio/x-scpls type, forming the file of the pls type of Winamp, you send.

*) Whether or not under present conditions "the access from PC" whether or not, User-Agent the value of user_agent_pc with it is done. Because of this, animated picture/sound can be enjoyed the program which corresponds to the Winamp play list by correlating to PLS or audio/x-scpls of the browser, probably will be. With Winamp, it is animated picture, you question, being to sow and not to go just a little regrettable.... /-. -playlist.pls? Http: /... With as for the type itself which is said it is the expectation which can be accessed even from MediaWiz and the like. Experimenting still, increases is perhaps, when utilizing the net radio, perhaps you can use effectively. Report we wait. Relation) http: //pc.2ch.net/test/read.cgi/avi/1071203538/405-413n
------------------
Postscript; As for wizd 0.12f MediaWiz in order to distinguish the type of file with the last extension of URL, with mounting the time before it was found that it does not go well. With wizd 0.12f, the fact that pls it adds to the end of URL which we would like to form with playlist is permitted, it tried trying to go well then due to the fact that wizd deletes this. Example)

Http: When it accesses //64.236.34.97:80/stream/1005, when the data flows, /-. -playlist.pls? Http: //64.236.34.97:80/stream/1005.pls (as for URL which is formed http: //64.236.34.97:80/stream/1005) Furthermore, when it does as follows, it can use the professional comb function of wizd. /-. -playlist.pls? /-. -http: //64.236.34.97:80/stream/1005.pls (as for URL which is formed http: / Server: 8000/-. -http: //64.236.34.97:80/stream/1005)------------------

-----
  ·mplayer Example
  With being the case that it is said, here mplayer (win32 edition) with it introduces the setting which uses IE on Win2k. Because UNIX being yourself from this explanation, it can arrange the person probably is, please persevere. ; As for the person that please stop -) and, it is not understood well.


First, mplayer (win32 edition) please download. Just a little size is large texture. 
Ftp: It develops //ftp.mplayerhq.hu/MPlayer/releases/win32-beta/mplayer-mingw32-1.0pre3.zip and this file, places in the suitable place. 

Here C: You suppose that it tried to become \mplayer\mplayer.exe.
Next, the correlation of extension PLS is modified. If (this meaning is not recognized, please abandon. It is dangerous, is) (well in the person who is accustomed simple story it is but) From file type of folder option, please choose extension PLS, open details setting. You reset to the origin, that when it has become, pushing that, when you reset, increases with details setting. When there is no PLS, please make anew. 

When details setting is opened, that you think whether there are some such as open and play. Because next, present setting is modified, just the person who has the self-confidence which can be reset please advance. Well, deleting, open (regardless it is good) please designate other things as one. And, that those which are left (open), it compiles. It correlates (to be plugged to the application which, here essence) C: \mplayer\mplayer.exe -playlist you insert "the %1". (Reference pushing, choosing, it is good to append, don't you think? is. ) At above, it is the expectation where it can correlate to extension pls, mplayer. 

Well, opening IE anew in this state, unless (so it does, it is not reflected...) please try accessing the address of wizd. After that, /-. -playlist.pls? Http.. it clicks whether. with in the avi file kana which has become. So, when it does, the DOS window opens and it is the expectation which mplayer plays back. When it is mplayer, like MediaWiz it plays back even with AVI, don't you think?. When (with WindowsMediaPlayer it tries to have) (as expected it is non-interleaved entirely locally, whether useless because) MPEG4v3 with it can play back, well, please try enjoying to play. By the way, the full screen of mplayer is the f key of the keyboard. Being there is no Alt+Enter note; -) To come out, it is q. Originally fast forward and the rewind <- -> are the key, but, mplayer this on the stream knowing, now me is the MediaWiz wind setting which used ? PC, but don't you think? when like this it tries doing, you know MediaWiz how is splendid,: -)-----


