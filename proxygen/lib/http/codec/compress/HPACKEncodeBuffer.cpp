/*
 *  Copyright (c) 2014, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <proxygen/lib/http/codec/compress/HPACKEncodeBuffer.h>

#include <proxygen/lib/http/codec/compress/HPACKConstants.h>
#include <proxygen/lib/http/codec/compress/Logging.h>

#include <ctype.h>
#include <memory>

using folly::IOBuf;
using std::string;
using std::unique_ptr;

namespace proxygen {

HPACKEncodeBuffer::HPACKEncodeBuffer(
  uint32_t growthSize,
  HPACK::MessageType msgType,
  bool huffman) :
    growthSize_(growthSize),
    buf_(&bufQueue_, growthSize),
    msgType_(msgType),
    huffman_(huffman) {
}

void HPACKEncodeBuffer::addHeadroom(uint32_t headroom) {
  // we expect that this function is called before any encoding happens
  CHECK(bufQueue_.front() == nullptr);
  // create a custom IOBuf and add it to the queue
  unique_ptr<IOBuf> buf = IOBuf::create(std::max(headroom, growthSize_));
  buf->advance(headroom);
  bufQueue_.append(std::move(buf));
}

void HPACKEncodeBuffer::append(uint8_t byte) {
  buf_.ensure(1);
  uint8_t* data = buf_.writableData();
  *data = byte;
  buf_.append(1);
}

uint32_t HPACKEncodeBuffer::encodeInteger(uint32_t value, uint8_t prefix,
                                          uint8_t nbit) {
  CHECK(nbit > 0 && nbit <= 8);
  uint32_t count = 0;
  uint8_t prefix_mask = HPACK::NBIT_MASKS[nbit];
  uint8_t mask = ~prefix_mask & 0xFF;

  // write the first byte
  uint8_t byte = prefix & prefix_mask;
  if (value < mask) {
    // fits in the first byte
    byte = byte | (mask & value);
    append(byte);
    return 1;
  }

  byte |= mask;
  value -= mask;
  ++count;
  append(byte);
  // variable length encoding
  while (value >= 128) {
    byte = 128 | (127 & value);
    append(byte);
    value = value >> 7;
    ++count;
  }
  // last byte, which should always fit on 1 byte
  append(value);
  ++count;
  return count;
}

uint32_t HPACKEncodeBuffer::encodeHuffman(const std::string& literal) {
  uint32_t size = huffman::getSize(literal, msgType_);
  // add the length
  uint32_t count = encodeInteger(size, HPACK::LiteralEncoding::HUFFMAN, 7);
  // ensure we have enough bytes before performing the encoding
  buf_.ensure(size);
  count += huffman::encode(literal, msgType_, buf_);
  return count;
}

uint32_t HPACKEncodeBuffer::encodeLiteral(const std::string& literal) {
  if (huffman_) {
    return encodeHuffman(literal);
  }
  // otherwise use simple layout
  uint32_t count =
    encodeInteger(literal.size(), HPACK::LiteralEncoding::PLAIN, 7);
  // copy the entire string
  buf_.ensure(literal.size());
  memcpy(buf_.writableData(), literal.c_str(), literal.size());
  buf_.append(literal.size());
  count += literal.size();
  return count;
}

string HPACKEncodeBuffer::toBin(uint8_t bytesPerLine) {
  return dumpBin(bufQueue_.front(), bytesPerLine);
}

}
