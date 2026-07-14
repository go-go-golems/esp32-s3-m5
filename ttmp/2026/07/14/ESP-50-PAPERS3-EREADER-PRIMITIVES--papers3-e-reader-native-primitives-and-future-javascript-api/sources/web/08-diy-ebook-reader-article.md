[View All Posts](https://www.atomic14.com/)

read

HELP [SUPPORT](https://www.atomic14.com/support/) MY WORK: If you're feeling flush then please stop by [Patreon](https://www.patreon.com/atomic14) Or you can make a one off donation via [ko-fi](https://ko-fi.com/atomic14)

![](https://www.youtube.com/watch?v=VLiCgB0odOQ)

Build your own DIY e-Reader with an ESP32, an e-paper display, and some simple coding. This project decodes ePUB files, parses HTML files, and renders texts and images on an e-paper display, while optimizing battery usage.

\[0:00\] Hey everyone I’ve been working on a new project with an e-paper display, it’s a DIY e-Reader.  
\[0:06\] It’s got an SD Card to hold the books - so it’s quite easy to add new ones and you can hold an awful  
  
\[0:11\] lot of books on it. There’s a simple UI to navigate through the files and we can open a book and read  
  
\[0:16\] it. I’ve built mine around a 4.7-inch e-paper display from LilyGo but you should be able to  
  
\[0:21\] get the software up and running on any ESP32 based e-paper board - provided it has PSRAM built-in and  
  
\[0:27\] a way to connect an SD Card. Lots of boards come with an SD Card reader but, if yours is missing one  
  
\[0:32\] then you can easily connect one using SPI following the instructions in this video. I’ve put  
  
\[0:37\] a link in the description. You’ll also need three buttons for navigation. We need UP, DOWN and SELECT.  
\[0:44\] The code is pretty flexible, so these can be defined as either active low or active high  
  
\[0:49\] depending on your board and your wiring. We’ll run through how this all works after this quick thanks  
  
\[0:53\] to PCBWay for sponsoring the channel. They’ve been supporting me for a while now and they’ve  
  
\[0:58\] been really helpful. I’ve made quite a few boards with them and the service has always been great.  
\[1:03\] They also do CNC and 3D printing which we’ll be trying out in some future videos. Check out the  
  
\[1:08\] link in the description. So how does it all work? Well let’s start off with a quick rough overview  
  
\[1:14\] of the ePUB format. This is a free and open format for eBooks that is compatible with most popular  
  
\[1:20\] eBook readers. This means there are a lot of free books available that you can download and read.  
  
\[1:25\] Now, I’ve only investigated the format sufficiently to get a basic reader up and running but there  
  
\[1:30\] is a detailed spec with a lot more information online. If we look at the first few bytes of an  
  
\[1:35\] ePUB file we can see that it’s actually a ZIP archive. We can easily change the extension of  
  
\[1:40\] the file to zip and then decompress it using unzip to see what’s inside. The most interesting file is  
  
\[1:46\] the content.opf file this XML file contains three sections that are of interest to us. The metadata  
  
\[1:53\] section contains useful information such as the title of the book along with the content to use  
  
\[1:58\] for the cover page image. The manifest contains a list of all the items contained in the ePUB file  
  
\[2:04\] and finally, the spine section lists the items in the order they should be displayed to the reader.  
\[2:09\] There is some other useful information but for a basic e-reader, we can safely ignore it.  
  
\[2:14\] If we look at the individual items that make up the book, we can see that we have some images  
  
\[2:19\] and some HTML files. Now the ePUB specification dictates that the HTML files should all be valid  
  
\[2:25\] XHTML - this will come in handy later when we try to parse them. So there are several challenges to solve:  
\[2:32\] we need to get some ePUB files onto our device, we need a ZIP library that will run on the ESP32 and  
  
\[2:38\] let us read files from the ePUB archive, we need to parse the content.opf file using an XML parser  
  
\[2:45\] and finally, we need to take the XHTML items parse them and lay them out so they can be rendered on  
  
\[2:51\] the e-paper display. To make the first part simple I’ve attached an SD Card to the board.  
  
\[2:57\] This is a slightly hacked together contraption there is an official adapter for the LilyGo board,  
\[3:02\] but using what we’ve learned in the previous video I’ve got an SD Card attached to some spare  
  
\[3:06\] GPIO pins. Unfortunately, these are the only spare GPIO pins on the board so we can’t use  
  
\[3:12\] the touchscreen interface but, this is probably a good thing as buttons mean we can take advantage  
  
\[3:17\] of deep sleep and save battery power. With a bunch of ePUB files copied onto the SD Card it’s pretty  
  
\[3:23\] straightforward to list the contents using a couple of C functions. We can filter out the ePUB  
  
\[3:27\] files by checking the extension and ignore any hidden files that start with a dot. Now there is  
  
\[3:32\] a nasty gotcha here for the unwary by default the file system code in the ESP-IDF doesn’t  
  
\[3:38\] support long file names. So you need to enable this option using menuconfig. If you don’t do this then  
  
\[3:44\] you’ll end up with some very weird and wacky names showing up when you list the directory contents.  
\[3:49\] With the list of ePUB file names collected, we need to open them up and parse the content.opf  
  
\[3:55\] file. There’s a nice library we can use called miniz which will read ZIP archives. I’ve encapsulated  
  
\[4:01\] this in a very simple wrapper that will either extract files to memory or extract them to a file  
  
\[4:06\] on the SD Card. To extract the contents.opf file we just use these couple of lines. The file we’re  
  
\[4:12\] extracting is generally quite small so we can extract it to memory without any problems. To parse  
  
\[4:17\] the XML file I’m using a library called TinyXML2 this is a nice C++ library that is lightweight  
  
\[4:24\] and works really nicely on the ESP32. With the XML parsed it’s quite simple to extract out the  
  
\[4:29\] information we need about the book. We can get the title and we can get the item that should be used  
  
\[4:33\] for the cover image. We use this to drive our book selection UI. To process the manifest and spine  
  
\[4:40\] we first read in the items from the manifest and create a map from the ID of the item to the item  
  
\[4:45\] path in the ZIP archive. We then run through the spine and collect the items in the required order.  
  
\[4:51\] So that’s our ePUB structure pretty much parsed. We now have enough information to show a list of  
  
\[4:55\] books with a thumbnail and title and for each book we know what order to render the HTML files in  
  
\[5:01\] This leads us nicely onto the next problem we need to parse and render the HTML and CSS files.  
\[5:08\] Doing this properly on an ESP32 would be pretty difficult even with the extra PSRAM.  
  
\[5:14\] So, we will need to cheat slightly. Electronic books don’t really have that much styling  
  
\[5:19\] so we can pretty much ignore the CSS files. They’re also designed to be displayed on various different  
  
\[5:24\] devices and screen resolutions - so the structure of the HTML tends to be fairly straightforward.  
  
\[5:30\] We’re really only interested in a few HTML tags. We’re looking for headers, divs and paragraph tags.  
  
\[5:37\] We’ll treat these tags as containing blocks of text. We also want to pick up bold and italic text  
  
\[5:42\] to get some formatting and we also want the image tags so we can show the inline images in the book.  
\[5:48\] Since we know that we’ll be getting valid XHTML we can use the TinyXML2 parser on the files.  
  
\[5:55\] We parse the XML and then we visit each tag in the document picking up any opening tags that  
  
\[6:01\] are of interest to us. If we see a new block tag then we start collecting text making sure we flag  
  
\[6:07\] any sections that are bold or italic. We do this until we hit the end of the block tag. If we hit  
  
\[6:13\] an image tag then we end the current block, add the image, and then create a new block. At the end of  
  
\[6:18\] this processing we have a list of text and image blocks in the order they appear in the HTML file.  
  
\[6:24\] We now need to fit these to the width of our display and calculate the height of each block so  
  
\[6:28\] we can work out where the page breaks should be. For images this is pretty simple: we get the dimensions  
  
\[6:34\] of the image and then scale them so they fit within the screen bounds. For the text blocks, it’s  
  
\[6:39\] a bit more complicated - we need to layout the text so it fits nicely within the width of the screen.  
  
\[6:45\] We need to work out where the line breaks should be. There’s a very nice dynamic programming  
  
\[6:49\] way of doing this that should ensure we end up with nicely laid out text without huge gaps or  
  
\[6:54\] really tiny gaps. You can watch a really interesting talk on dynamic programming here that  
  
\[6:59\] I have linked to in the description that describes the algorithm in detail. With our text blocks  
  
\[7:04\] broken into individual lines and our image heights all calculated we can now simply assign the lines  
  
\[7:09\] of text and images to pages. We start assigning text and images to a page and when we run out  
  
\[7:14\] of space, we just start a new page. When it comes to rendering the pages it’s now pretty simple.  
  
\[7:20\] We just draw the images and the words where they’ve been laid out. All the heavy lifting  
  
\[7:24\] has already been done so our rendering is easy. To make the device portable I’ve got a battery  
  
\[7:29\] attached and the code goes into deep sleep after 30 seconds of user inactivity. I’ve got a video  
  
\[7:34\] that dives into deep sleep that’s definitely worth watching. All the code is on GitHub there’s  
  
\[7:38\] huge scope for improving what’s there the project is by no means done and still needs a  
  
\[7:44\] considerable amount of work. So please contribute if you’ve got an ESP32 and an e-paper display  
  
\[7:49\] then let me know if you get it to work. I’m really excited to see what can be done. Good luck!

HELP [SUPPORT](https://www.atomic14.com/support/) MY WORK: If you're feeling flush then please stop by [Patreon](https://www.patreon.com/atomic14) Or you can make a one off donation via [ko-fi](https://ko-fi.com/atomic14)

Want to keep up to date with the latest posts and videos? [Subscribe to the newsletter](https://atomic14.substack.com/)