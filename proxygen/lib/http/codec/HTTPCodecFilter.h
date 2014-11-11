/*
 *  Copyright (c) 2014, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <proxygen/lib/http/codec/HTTPCodec.h>
#include <proxygen/lib/utils/FilterChain.h>

namespace proxygen {

typedef GenericFilter<
  HTTPCodec,
  HTTPCodec::Callback,
  &HTTPCodec::setCallback,
  true> HTTPCodecFilter;

/**
 * An implementation of HTTPCodecFilter that passes through all calls. This is
 * useful to subclass if you aren't interested in intercepting every function.
 * See HTTPCodec.h for documentation on these methods.
 */
class PassThroughHTTPCodecFilter: public HTTPCodecFilter {
 public:
  /**
   * By default, the filter gets both calls and callbacks
   */
  explicit PassThroughHTTPCodecFilter(bool calls = true,
                                      bool callbacks = true):
      HTTPCodecFilter(calls, callbacks) {}

  // HTTPCodec::Callback methods
  void onMessageBegin(StreamID stream, HTTPMessage* msg) override;

  void onPushMessageBegin(StreamID stream, StreamID assocStream,
                          HTTPMessage* msg) override;

  void onHeadersComplete(StreamID stream,
                         std::unique_ptr<HTTPMessage> msg) override;

  void onBody(StreamID stream,
              std::unique_ptr<folly::IOBuf> chain) override;

  void onChunkHeader(StreamID stream, size_t length) override;

  void onChunkComplete(StreamID stream) override;

  void onTrailersComplete(StreamID stream,
                          std::unique_ptr<HTTPHeaders> trailers) override;

  void onMessageComplete(StreamID stream, bool upgrade) override;

  void onError(StreamID stream,
               const HTTPException& error,
               bool newStream = false) override;

  void onAbort(StreamID stream,
               ErrorCode code) override;

  void onGoaway(uint64_t lastGoodStreamID,
                ErrorCode code) override;

  void onPingRequest(uint64_t uniqueID) override;

  void onPingReply(uint64_t uniqueID) override;

  void onWindowUpdate(StreamID stream, uint32_t amount) override;

  void onSettings(const SettingsList& settings) override;

  void onSettingsAck() override;

  uint32_t numOutgoingStreams() const override;

  uint32_t numIncomingStreams() const override;

  // HTTPCodec methods
  CodecProtocol getProtocol() const override;

  TransportDirection getTransportDirection() const override;

  bool supportsStreamFlowControl() const override;

  bool supportsSessionFlowControl() const override;

  StreamID createStream() override;

  void setCallback(HTTPCodec::Callback* callback) override;

  bool isBusy() const override;

  void setParserPaused(bool paused) override;

  size_t onIngress(const folly::IOBuf& buf) override;

  void onIngressEOF() override;

  bool isReusable() const override;

  bool isWaitingToDrain() const override;

  bool closeOnEgressComplete() const override;

  bool supportsParallelRequests() const override;

  bool supportsPushTransactions() const override;

  void generateHeader(folly::IOBufQueue& writeBuf,
                      StreamID stream,
                      const HTTPMessage& msg,
                      StreamID assocStream,
                      HTTPHeaderSize* size) override;

  size_t generateBody(folly::IOBufQueue& writeBuf,
                      StreamID stream,
                      std::unique_ptr<folly::IOBuf> chain,
                      bool eom) override;

  size_t generateChunkHeader(folly::IOBufQueue& writeBuf,
                             StreamID stream,
                             size_t length) override;

  size_t generateChunkTerminator(folly::IOBufQueue& writeBuf,
                                 StreamID stream) override;

  size_t generateTrailers(folly::IOBufQueue& writeBuf,
                          StreamID stream,
                          const HTTPHeaders& trailers) override;

  size_t generateEOM(folly::IOBufQueue& writeBuf,
                     StreamID stream) override;

  size_t generateRstStream(folly::IOBufQueue& writeBuf,
                           StreamID stream,
                           ErrorCode statusCode) override;

  size_t generateGoaway(folly::IOBufQueue& writeBuf,
                        StreamID lastStream,
                        ErrorCode statusCode) override;

  size_t generatePingRequest(folly::IOBufQueue& writeBuf) override;

  size_t generatePingReply(folly::IOBufQueue& writeBuf,
                           uint64_t uniqueID) override;

  size_t generateSettings(folly::IOBufQueue& writeBuf) override;

  size_t generateWindowUpdate(folly::IOBufQueue& writeBuf,
                              StreamID stream,
                              uint32_t delta) override;

  HTTPSettings* getEgressSettings() override;

  const HTTPSettings* getIngressSettings() const override;

  void setHeaderCodecStats(HeaderCodec::Stats* stats) override;

  void enableDoubleGoawayDrain() override;

  HTTPCodec::StreamID getLastIncomingStreamID() const override;
};

typedef FilterChain<
  HTTPCodec,
  HTTPCodec::Callback,
  PassThroughHTTPCodecFilter,
  &HTTPCodec::setCallback,
  true> HTTPCodecFilterChain;

}
