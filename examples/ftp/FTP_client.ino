/* 20190113V7-FTP Client
 * There are a number of FTP server libraries available for arduino but google
 * for an ardiuno/esp8266 FTP client and all roads lead back to surfer tim:
 * http://playground.arduino.cc/Code/FTP
 *
 * I found Patrick Duensing's (fryguy128) work - he modified it for SPIFFS
 * https://github.com/fryguy128/esp8266FTP
 *
 * So I downloaded it, tried it and it didn't work!!
 *
 * After some head-scratching I realised that it was trying to upload the file
 * to the root folder on my NAS - hence the "permission denied" problems I was
 * getting.
 *
 * More googling on FTP and learn't a lot more on how it works. From that it
 * was a relatively trivial exercise to add CWD to the original
 * code to change the destination folder to /home for the specified
 * user.
 *
 * But, having started, I didn't stop there:
 *
 *    To put it mildly, I dislike functions that are dependent on global variables to
 *    work, this revised version removes all dependence on global variables.
 *
 *    I've added a LOT more comments on what's happening - for my own benefit,
 *    memory like a sieve!
 *
 *    Instead of the original function doFTP() returning a simple YES/NO answer on
 *    completion, this version returns the server return code -  useful for dumping
 *    errors into an error.log for a post-mortem after the event when you don't have
 *    a serial output to stare at.
 *    The error codes are the 'standard' FTP error codes.
 *
 * To use only two functions are required:
 *
 *    doFTP and eRcv
 *
 * AMH 15 Jan 2019
 *
 */

/*------------------------------------------------------
 * LIBRARIES
 --------------------------------------------------------*/

#include <ESP8266WiFi.h>
#include <FS.h>

/*------------------------------------------------------
 * CONSTANTS
 --------------------------------------------------------*/

const char *ssid = "";
const char *wifipwd = "";

/*------------------------------------------------------
 * VARIABLES global scope
 --------------------------------------------------------*/

// FTP server details
char *xhost = "localhost"; // synology diskstation
char *xusername = "manuel";
char *xpassword = "testpass";
char *xfolder = "";
/*destination folder on FTP server
this is an optional parameter with the function prototype
defined below. To leave the default folder on the server
unchanged, either enter "" for xfolder or omit this param in
the function call in doFTP() */

// file to upload
char *xfilename = "data.txt";

short FTPresult; // outcome of FTP upload

/*------------------------------------------------------
 * OBJECTS
 --------------------------------------------------------*/

// none

/*------------------------------------------------------
 * SETUP
 --------------------------------------------------------*/

// Function prototype - required if folder is an optional argument
short doFTP(char *, char *, char *, char *, char * = "");

void setup()
{

  // a one-time test to check it works

  Serial.begin(115200);
  delay(100);

  // Wifi Setup
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, wifipwd);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("----------------------------------------");

  // SPIFF setup
  SPIFFS.begin();

  // assumes that SPIFFS exists and is formatted

  // Open test file, write sample data to it
  File f = SPIFFS.open(xfilename, "a");
  if (!f)
  {
    Serial.println("file open failed");
    Serial.println("Sketch terminating...");
  }
  else
  {
    Serial.println("file data.txt exists in SPIFFS");
    // write sample data
    f.println("Write this line into test file");
    f.close();
    // get and print contents of root folder
    String str = "";
    Dir dir = SPIFFS.openDir("");
    while (dir.next())
    {
      str += dir.fileName();
      str += " / ";
      str += dir.fileSize();
      str += "\r\n";
    }
    Serial.print(str);
    Serial.println("--------------------------------");

    // Attempt FTP upload
    FTPresult = doFTP(xhost, xusername, xpassword, xfilename, xfolder);
    // What is the outcome?
    Serial.println("A return code of 226 is success");
    Serial.print("Return code = ");
    Serial.println(FTPresult);
  }
} // end setup

/*------------------------------------------------------
 * MAIN LOOP
 --------------------------------------------------------*/
void loop()
{
  // there isn't one - a one time test
}

/*------------------------------------------------------
 * FUNCTION - doFTP
 * Connects to a FTP server and transfers a file. Connection
 * is established using the FTP commands/responses defined at
 * https://en.wikipedia.org/wiki/List_of_FTP_commands or in
 * more detail at http://www.nsftools.com/tips/RawFTP.htm
 *
 * Parameters passed:
 *   host - the IP address of the FTP server
 *   uname - username for the account on the server
 *   pwd - user password
 *   filename - the file to be transferred
 *   folder (optional) - folder on the server where
 *   the file will be copied. This is may be omitted if
 *   the file is to be copied into the default folder on
 *   the server.
 *
 * Return codes:
 *    226 - a successful transfer
 *    400+ - any return code greater than 400 indicates
 *    an error. These codes are defined at
 *    https://en.wikipedia.org/wiki/List_of_FTP_server_return_codes
 *    Exceptions to this are:
 *    900 - failed to open file on SPIFFS
 *    910 - failed to connect to server
 *
 * Dependencies:
 *   Libraries - <ESP8266WiFi.h> wifi library
 *               <FS.h> SPIFFS library
 *   Functions - eRcv
 --------------------------------------------------------*/
short doFTP(char *host, char *uname, char *pwd, char *fileName, char *folder)
{
  WiFiClient ftpclient;
  WiFiClient ftpdclient;

  const short FTPerrcode = 400; // error codes are > 400
  const byte Bufsize = 128;
  char outBuf[Bufsize];
  short FTPretcode = 0;
  const byte port = 21; // 21 is the standard connection port

  File ftx = SPIFFS.open(fileName, "r"); // file to be transmitted
  if (!ftx)
  {
    Serial.println(F("file open failed"));
    return 900;
  }
  if (ftpclient.connect(host, port))
  {
    Serial.println(F("Connected to FTP server"));
  }
  else
  {
    ftx.close();
    Serial.println(F("Failed to connect to FTP server"));
    return 910;
  }
  FTPretcode = eRcv(ftpclient, outBuf, Bufsize);
  if (FTPretcode >= 400)
    return FTPretcode;

  /* User - Authentication username
   * Send this command to begin the login process. username should be a
   * valid username on the system, or "anonymous" to initiate an anonymous login.
   */
  ftpclient.print("USER ");
  ftpclient.println(uname);
  FTPretcode = eRcv(ftpclient, outBuf, Bufsize);
  if (FTPretcode >= 400)
    return FTPretcode;

  /* PASS - Authentication password
   * After sending the USER command, send this command to complete
   * the login process. (Note, however, that an ACCT command may have to be
   * used on some systems, not needed with synology diskstation)
   */
  ftpclient.print("PASS ");
  ftpclient.println(pwd);
  FTPretcode = eRcv(ftpclient, outBuf, Bufsize);
  if (FTPretcode >= 400)
    return FTPretcode;

  // CWD - Change the working folder on the FTP server
  if (!(folder == ""))
  {
    ftpclient.print("CWD ");
    ftpclient.println(folder);
    FTPretcode = eRcv(ftpclient, outBuf, Bufsize);
    if (FTPretcode >= 400)
    {
      return FTPretcode;
    }
  }

  /* SYST - Returns a word identifying the system, the word "Type:",
   * and the default transfer type (as would be set by the
   * TYPE command). For example: UNIX Type: L8 - this is what
   * the diskstation returns
   */
  ftpclient.println("SYST");
  FTPretcode = eRcv(ftpclient, outBuf, Bufsize);
  if (FTPretcode >= 400)
    return FTPretcode;

  /* TYPE - sets the transfer mode
   * A - ASCII text
   * E - EBCDIC text
   * I - image (binary data)
   * L - local format
   * for A & E, second char is:
   * N - Non-print (not destined for printing). This is the default if
   * second-type-character is omitted
   * Telnet format control (<CR>, <FF>, etc.)
   * C - ASA Carriage Control
   */
  ftpclient.println("Type I");
  FTPretcode = eRcv(ftpclient, outBuf, Bufsize);
  if (FTPretcode >= 400)
    return FTPretcode;

  /* PASV - Enter passive mode
   * Tells the server to enter "passive mode". In passive mode, the server
   * will wait for the client to establish a connection with it rather than
   * attempting to connect to a client-specified port. The server will
   * respond with the address of the port it is listening on, with a message like:
   * 227 Entering Passive Mode (a1,a2,a3,a4,p1,p2), e.g. from diskstation
   * Entering Passive Mode (192,168,0,5,217,101)
   */
  ftpclient.println("PASV");
  FTPretcode = eRcv(ftpclient, outBuf, Bufsize);
  if (FTPretcode >= 400)
    return FTPretcode;
  /* This is parsing the return from the server
   * where a1.a2.a3.a4 is the IP address and p1*256+p2 is the port number.
   */
  char *tStr = strtok(outBuf, "(,"); // chop the output buffer into tokens based on the delimiters
  int array_pasv[6];
  for (int i = 0; i < 6; i++)
  {                             // there are 6 elements in the address to decode
    tStr = strtok(NULL, "(,");  // 1st time in loop 1st token, 2nd time 2nd token, etc.
    array_pasv[i] = atoi(tStr); // convert to int, why atoi - because it ignores any non-numeric chars
                                // after the number
    if (tStr == NULL)
    {
      Serial.println(F("Bad PASV Answer"));
    }
  }
  // extract data port number
  unsigned int hiPort, loPort;
  hiPort = array_pasv[4] << 8;  // bit shift left by 8
  loPort = array_pasv[5] & 255; // bitwise AND
  Serial.print(F("Data port: "));
  hiPort = hiPort | loPort; // bitwise OR
  Serial.println(hiPort);
  // first instance of dftp
  if (ftpdclient.connect(host, hiPort))
  {
    Serial.println(F("Data port connected"));
  }
  else
  {
    Serial.println(F("Data connection failed"));
    ftpclient.stop();
    ftx.close();
  }

  /* STOR - Begin transmission of a file to the remote site. Must be preceded
   * by either a PORT command or a PASV command so the server knows where
   * to accept data from
   */
  ftpclient.print("STOR ");
  ftpclient.println(fileName);
  FTPretcode = eRcv(ftpclient, outBuf, Bufsize);
  if (FTPretcode >= 400)
  {
    ftpdclient.stop();
    return FTPretcode;
  }
  Serial.println(F("Writing..."));

  byte clientBuf[64];
  int clientCount = 0;

  while (ftx.available())
  {
    clientBuf[clientCount] = ftx.read();
    clientCount++;
    if (clientCount > 63)
    {
      ftpdclient.write((const uint8_t *)clientBuf, 64);
      clientCount = 0;
    }
  }
  if (clientCount > 0)
    ftpdclient.write((const uint8_t *)clientBuf, clientCount);
  ftpdclient.stop();
  Serial.println(F("Data disconnected"));
  FTPretcode = eRcv(ftpclient, outBuf, Bufsize);
  if (FTPretcode >= 400)
  {
    return FTPretcode;
  }

  // End the connection
  ftpclient.println("QUIT");
  ftpclient.stop();
  Serial.println(F("Disconnected from FTP server"));

  ftx.close();
  Serial.println(F("File closed"));
  return FTPretcode;
} // end function doFTP
/*------------------------------------------------------
 * FUNCTION - eRcv
 * Reads the response from an FTP server and stores the
 * output in a buffer.Extracts the server return code from
 * the buffer.
 *
 * Parameters passed:
 *   aclient - a wifi client connected to FTP server and
 *   delivering the server response
 *   outBuf - a buffer to store the server response on
 *   size - size of the buffer in bytes
 *
 * Return codes:
 *    These are the first three chars in the buffer and are
 *    defined in
 *    https://en.wikipedia.org/wiki/List_of_FTP_server_return_codes
 *
 * Dependencies:
 *   Libraries - <ESP8266WiFi.h> wifi library
 *   Functions - none
 --------------------------------------------------------*/
short eRcv(WiFiClient aclient, char outBuf[], int size)
{
  byte thisByte;
  char index;
  String respStr = "";
  while (!aclient.available())
    delay(1);
  index = 0;
  while (aclient.available())
  {
    thisByte = aclient.read();
    Serial.write(thisByte);
    if (index < (size - 2))
    { // less 2 to leave room for null at end
      outBuf[index] = thisByte;
      index++;
    }
  }                  // note if return from server is > size it is truncated.
  outBuf[index] = 0; // putting a null because later strtok requires a null-delimited string
  // The first three bytes of outBuf contain the FTP server return code - convert to int.
  for (index = 0; index < 3; index++)
  {
    respStr += (char)outBuf[index];
  }
  return respStr.toInt();
} // end function eRcv
