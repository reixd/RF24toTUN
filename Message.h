/*
 * The MIT License (MIT)
 * Copyright (c) 2014 Rei <devel@reixd.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *
 */

#include <cstdint>
#include <string>
#include <vector>


/**
* This class encapsulates the messages received from the TUN/TAP or radio interface
*/
class Message {
  public:
    Message() :
        payload_({0}),
        length_(0),
        seqNo_(0) {};

    /**
    * Get the length of the message in bytes.
    * @return The byte size of the message
    */
    std::size_t getLength() {
        return length_;
    };

    /**
    * Get the sequence number of the message
    * @return The sequence number
    */
    uint8_t getSeqNo() {
        return seqNo_;
    };

    /**
    * Get the payload of the message as a std::string
    * @return the payload as string
    */
    std::string getPayloadStr() {
        return std::string(payload_.begin(), payload_.end());
    };

    /**
    * Get the payload of the message as a char array
    * @return the payload as a c-string
    */
    const char* getPayload() {
        const char* res = payload_.data();
        return res;
    };

    /**
    * Set the sequence number of the message
    * @param sn The new sequence number
    */
    void setSeqNo(uint8_t sn) {
        seqNo_ = sn;
    }

    /**
    * Set the payload of the message
    * @param buffer char array as the payload
    * @param bsize size of the buffer
    */
    void setPayload(char * buffer, std::size_t bsize) {
        payload_.clear();
        payload_.assign(buffer, buffer + bsize);
        length_ = payload_.size();
    };

  private:
    std::vector<char> payload_;  /**< Internal vector data structure to save the payload*/
    std::size_t length_;  /**< Current length of the payload */
    uint8_t seqNo_;  /**< Sequence number */

};
