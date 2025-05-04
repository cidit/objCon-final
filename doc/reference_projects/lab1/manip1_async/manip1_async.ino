#include <HardwareSerial.h>
 
constexpr char END_CHAR = '\n';
constexpr size_t BUFF_LEN = 256;
 
HardwareSerial remote = HardwareSerial(2),
               local = HardwareSerial(Serial);
byte local_wbuffer[BUFF_LEN] = { 0 },
     remote_wbuffer[BUFF_LEN] = { 0 },;
size_t local_windex = 0, remote_windex;
 
 
void setup() {
  local.begin(9600);             //init communication over USB
  remote.begin(9600);            //communication over RX/TX pins
  pinMode(LED_BUILTIN, OUTPUT);  // initialize digital pin LED_BUILTIN as an output.
}
 
void loop() {
  // we can reuse the out buffer, and we dont care about the garbage it's initialized with
  static byte out_buffer[BUFF_LEN];
 
  if (bit_received(local)) {
    digitalWrite(LED_BUILTIN, HIGH);
    auto done = read_async(local, local_wbuffer, local_windex, out_buffer);
    digitalWrite(LED_BUILTIN, LOW);
 
    if (done) {
      auto stringified = String(out_buffer);
      remote.println(stringified);
    }
  }
 
  if (bit_received(remote)) {
    digitalWrite(LED_BUILTIN, HIGH);
    auto done = read_async(remote, remote_wbuffer, remote_windex, out_buffer);
    digitalWrite(LED_BUILTIN, LOW);
 
    if (done) {
      auto stringified = String(out_buffer);
      remote.println(stringified);
    }
  }
}
 
 
/**
 * reads the specified source asynchronously.
 * buffers must be big enough.
 * expects end of serial input to be a newline char. replaces it by a cstring terminator
 *
 * @param src reference to the source to read from
 * @param working_buffer the internally handled buffer used to handle reading (never use directly, must be big enough)
 * @param working_index the internally handled index of the working buffer (never use directly)
 * @param output_buffer the buffer in which the contents of the transmission will be written
 * @return whether we have finished reading and the `output_buffer` is ready to be read or not
 */
bool read_async(HardwareSerial& src, byte& working_buffer, size_t& working_index, byte& output_buffer) {
  // if we cant read anyways, GTFO.
  if (!src.available()) return false;
 
  // overwrite current index with the latest byte
  working_buffer[working_index] = src.read();
 
  // if we havent found our ending character, exit early with false to signify we're not done reading.
  if (working_buffer[working_index] != END_CHAR) {
    working_index +=1;
    return false;
  }
 
  // copy the content of the working buffer to the output buffer. could be replaced by memcpy but didnt to keep it simple.
  for (size_t i = 0; i < working_index; i++) {
    output_buffer[i] = working_buffer[i];
  }
  // add a string terminator at the end of the output buffer. (to make it a valid cstring)
  output_buffer[working_index] = '\0';
 
  // reset the working index and return true to say that we've finished reading and that the output buffer is ready to be used.
  working_index = 0;
  return true;
}
 
bool bit_received(HardwareSerial& src) {
  return src.available();
}