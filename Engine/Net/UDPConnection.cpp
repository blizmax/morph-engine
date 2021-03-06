﻿#include "UDPConnection.hpp"
#include "Engine/Net/NetPacket.hpp"
#include "Engine/Net/UDPSession.hpp"
#include "Engine/Debug/Log.hpp"
#include "Engine/Math/Cyclic.hpp"

span<const uint16_t> UDPConnection::PacketTracker::reliables() const {
  EXPECTS(mSentReliableCount <= MAX_RELIABLES_PER_PACKET);
  return span<const uint16_t>{ mSentReliable.data(), mSentReliableCount };
}

void UDPConnection::PacketTracker::bind(const NetPacket& packet) {
  sendSec = GetCurrentTimeSeconds();
  ack = packet.ack();

  auto reliableMsgs = packet.messagesReliable();
  mSentReliableCount = 0;
  for(const NetMessage* msg: reliableMsgs) {
    registerReliable(msg);
  }

  isUsing = true;
}

void UDPConnection::PacketTracker::registerReliable(const NetMessage* msg) {
  mSentReliable[mSentReliableCount] = msg->reliableId();
  mSentReliableCount++;
}

UDPConnection::~UDPConnection() {
  if(valid()) {
    flush(true);
  }
}

void UDPConnection::invalidate() {
  mOwner = nullptr;
  mNetObjectViews.NetObject::ViewCollection::~ViewCollection();
  new(this) UDPConnection();
}
bool UDPConnection::send(NetMessage& msg) {
  mOwner->finalize(msg);

  if (msg.inorder()) {
    auto& channel = mMessageChannels[msg.definition()->channelIndex];
    msg.sequenceId(channel.nextSendSequenceId);
    channel.nextSendSequenceId++;
  }

  if(msg.reliable()) {
    mUnsentReliable.push_back(msg);
  } else {
    mOutboundUnreliables.push_back(msg);
  }

  return true;
}

bool UDPConnection::flush(bool force) {
  if (!valid()) return false;
  tryAppendHeartbeat();
  if(!force) {
    if (!shouldSendPacket()) {
      return false;
    }
  }

  NetPacket packet;

  packet.begin(*this);

  mSentReliable.reserve(mSentReliable.size() + mUnsentReliable.size());

  std::sort(mSentReliable.begin(), mSentReliable.end(), [](NetMessage& a, NetMessage& b) {
    return a.lastSendSec() > b.lastSendSec();
  });
  {
    auto current = mSentReliable.rbegin();
    if(!mSentReliable.empty()) {
      mOldestUnconfirmedRelialbeId = current->reliableId();
    }
    while(current != mSentReliable.rend()) {
      if( cycLess(current->reliableId(), mOldestUnconfirmedRelialbeId) ) {
        mOldestUnconfirmedRelialbeId = current->reliableId();
      }

      double time = current->secondAfterLastSend();
      if ( time > DEFAULT_RELIABLE_RESEND_SEC) {
        bool appended = 
          packet.append(*current);

        if(appended) {
          current->lastSendSec() = GetCurrentTimeSeconds();
        }
      }
      ++current;
    }
  }

  {
    while (!mUnsentReliable.empty() && canSendNewReliable()) {
      mSentReliable.push_back(mUnsentReliable.back());
      NetMessage& current = mSentReliable.back();

      bool appended =
        packet.append(current);

      if(!appended) {
        mSentReliable.pop_back();
        break;
      } else {
        current.reliableId(mNextReliableId);
        mNextReliableId++;
        current.lastSendSec() = GetCurrentTimeSeconds();
        mUnsentReliable.pop_back();
      }
    }
  }

  {
    auto current = mOutboundUnreliables.begin();
    while(current != mOutboundUnreliables.end()) {
      bool appended = 
        packet.append(*current);
      
      if(!appended) {
        break;
      }

      ++current;
    }
  }

  {
    NetMessage objectSync(NETMSG_OBJECT_UPDATE);
    size_t leftSpace = std::min(packet.avaliabeSpace(), objectSync.capacity());
    mOwner->finalize(objectSync);
    if(leftSpace - objectSync.headerSize() < leftSpace) {
      leftSpace -= objectSync.headerSize();
      mNetObjectViews.sort();

      // This is the view for this connection; 
      auto views = mNetObjectViews.views();
      auto iter = views.begin();

      uint16_t appendedObjCount = 0;

      uint64_t curTime = mOwner->sessionTimeMs();

      objectSync << appendedObjCount;

      while(iter != views.end()) {
        size_t requireSpace = sizeof(net_object_id_t) + iter->ref->latestSnapshot().size +  sizeof(Timestamp) + sizeof(uint16_t);
        if(leftSpace < requireSpace) break;

        leftSpace -= requireSpace;
        NetObject::snapshot_t latestSnapshot = iter->ref->latestSnapshot();
        iter->lastUpdate.stamp = curTime;
        // NetObject::View* refView = mOwner->selfConnection()->netObjectViewCollection().find(iter->ref);
        // if(refView->lastUpdate.stamp > iter->lastUpdate.stamp) {
        //   memcpy(iter->snapshot.data, refView->snapshot.data, iter->snapshot.size);
        //   iter->lastUpdate.stamp = refView->lastUpdate.stamp;
        // }

        // Log::logf("sync net obj: %u", iter->ref->id);

        objectSync << iter->ref->id;
        objectSync << iter->lastUpdate;
        objectSync << iter->ref->latestSnapshot().size;

        NetObjectManager::NetObjectDefinition* def = mOwner->netObjectManager().type(iter->ref);
        def->sendSync(objectSync, latestSnapshot.data);

        ++iter;
        appendedObjCount++;
      }

      if(appendedObjCount > 0) {
        Log::logf("total object sync: %u", appendedObjCount);
        uint size = (uint)objectSync.size();
        objectSync.seekw(0, BytePacker::SEEK_DIR_BEGIN);
        objectSync << appendedObjCount;
        objectSync.seekw(size, BytePacker::SEEK_DIR_BEGIN);
        packet.append(objectSync);
      }
    }
  }

  packet.end();

  track(packet);

  mOwner->send(mIndexOfSession, packet);

  increaseAck();
  mLastSendSec = GetCurrentTimeSeconds();

  mOutboundUnreliables.clear();

  return true;
}

bool UDPConnection::set(UDPSession& session, uint8_t index, const NetAddress& addr) {
  mOwner = &session;
  mIndexOfSession = index;
  mAddress = addr;
  mHeartBeatTimer.flush();
  mLastReceivedSec = GetCurrentTimeSeconds();
  return true;
}

float UDPConnection::rtt(double rtt) {
  mRtt = rtt;
  return float(mRtt);
}

uint16_t UDPConnection::previousReceivedAckBitField() const {
  return mReceivedBitField;
}

bool UDPConnection::process(NetMessage& msg, UDPSession::Sender& sender) {
  if(msg.reliable()) {
    uint16_t reliableId = msg.reliableId();
    bool processed = isReliableReceived(reliableId);

    if(!processed) {
      if(cycGreater(reliableId, mHighestReceivedReliableId)) {

        uint16_t minId = mHighestReceivedReliableId - RELIALBE_WINDOW_SIZE + 1;

        mHighestReceivedReliableId = reliableId;
        mReceivedReliableMessage.set(reliableId, true);

        uint16_t maxId = mHighestReceivedReliableId - RELIALBE_WINDOW_SIZE + 1;

        for(uint16_t i = minId; cycLess(i, maxId); i++) {
          mReceivedReliableMessage.set(i, false);
        }
      } else {
        mReceivedReliableMessage.set(reliableId, true);
      }

      if(msg.inorder()) {
        auto& channel = mMessageChannels[msg.definition()->channelIndex];

        if(cycLess(msg.sequenceId(), channel.nextExpectReceiveSequenceId)) {
          Log::logf("received sequence id[%u]\texpect[%u], throw away", msg.sequenceId(), channel.nextExpectReceiveSequenceId);
        }

        if(msg.sequenceId() == channel.nextExpectReceiveSequenceId) {
          mOwner->handle(msg, sender);
          channel.nextExpectReceiveSequenceId++;

          while(!channel.outOfOrderMessages.empty()) {
            bool hasprocessed = false;
            for (uint i = (uint)channel.outOfOrderMessages.size() - 1; i < (uint)channel.outOfOrderMessages.size(); --i) {
              auto& message = channel.outOfOrderMessages[i];

              if (message.sequenceId() == channel.nextExpectReceiveSequenceId) {
                mOwner->handle(message, sender);
                channel.nextExpectReceiveSequenceId++;

                message = channel.outOfOrderMessages.back();
                channel.outOfOrderMessages.pop_back();

                hasprocessed = true;
                break;
              }
            }

            if (!hasprocessed) break;
          }
        }

        if(cycGreater(msg.sequenceId(), channel.nextExpectReceiveSequenceId)) {
          // Log::logf("message with sequenceid[%u] is out of order, saved", msg.sequenceId());
          channel.outOfOrderMessages.push_back(msg);
        }
      } else {
        mOwner->handle(msg, sender);
      }
    }

    return !processed;
  } else {
    mOwner->handle(msg, sender);
  }

  return true;
}

bool UDPConnection::updateView(NetObject* netObject, net_object_snapshot_t* /*snapshot*/, uint64_t lastUpdate) {
  NetObject::View* view = mNetObjectViews.find(netObject);
  ENSURES(view != nullptr);

  if(view->lastUpdate.stamp > lastUpdate) return false;

  // memcpy(view->snapshot.data, snapshot, view->snapshot.size);
  view->lastUpdate.stamp = mOwner->sessionTimeMs();

  return true;
}

void UDPConnection::heartbeatFrequency(double freq) {
  EXPECTS(freq != 0);
  mHeartBeatTimer.flush();
  mHeartBeatTimer.duration = 1.0 / freq;
}

double UDPConnection::tickFrequency(double freq) {
  mTickSec = 1.0 / freq;
  return mTickSec;
}

bool UDPConnection::onReceive(const NetPacket::header_t& header) {
  mLastReceivedSec = GetCurrentTimeSeconds();

  {
    // process bit field, confirm packets
    uint16_t bf = header.previousReceivedAckBitField;
    uint16_t ack = header.lastReceivedAck;
 
    uint received = 1;
    while(bf > 0 || received) {
      if(received) {
        confirmReceived(ack);
      }
      ack--;
      received = bf & 0x1;
      bf >>= 1;
    }
  }


  {
    uint16_t ack = header.ack;
    // update the data which will carry out in the next packet
    if (cycLessEq(mLargestReceivedAck, ack)) {
      uint shift = ack - mLargestReceivedAck;
      mReceivedBitField <<= shift;
      uint16_t mask = 0x1 << (shift - 1u);
      mReceivedBitField = mReceivedBitField | mask;
      mLargestReceivedAck = ack;
    } else {
      uint shift = mLargestReceivedAck - ack - 1;
      uint16_t mask = 0x1 << shift;
      mReceivedBitField = mReceivedBitField | mask;
    }
  }

  return true;
}

bool UDPConnection::isReliableReceived(uint16_t reliableId) const {
  uint16_t minId = mHighestReceivedReliableId - RELIALBE_WINDOW_SIZE + 1;

  uint16_t maxId = mHighestReceivedReliableId;

  if(cycLess(reliableId, minId)) {
    return true;
  }

  if(cycGreater(reliableId, maxId)) {
    return false;
  }

  return mReceivedReliableMessage.test(reliableId);
}

size_t UDPConnection::pendingReliableCount() const {
  return mSentReliable.size() + mUnsentReliable.size();
}

void UDPConnection::connectionState(eConnectionState state) {
  mConnectionState = state;
}

void UDPConnection::disconnect() {
  connectionState(CONNECTION_DISCONNECTED);
  flush(true);
  invalidate();

}

uint16_t UDPConnection::increaseAck() {
  mNextAckToUse = mNextAckToUse + 1u;

  if(mNextAckToUse % PACKET_TRACKER_CACHE_SIZE == 0) {
    uint lost = 0;

    for(bool marker: mLostPacketMarker) {
      if(marker) {
        lost++;
      }
    }

    mLostRate = float(lost) / float(PACKET_TRACKER_CACHE_SIZE);
    mLostPacketMarker.fill(false);
  }

  return mNextAckToUse;
}

UDPConnection::PacketTracker& UDPConnection::track(NetPacket& packet) {

  mLastSendAck = packet.ack();

  uint index = mLastSendAck % PACKET_TRACKER_CACHE_SIZE;
 
  PacketTracker& tracker = mTrackers[index];

  if(tracker.occupied()) {
    // handle lost
    mLostPacketMarker[index] = true;
  }

  tracker.bind(packet);

  return tracker;
}

bool UDPConnection::shouldSendPacket() const {
  // bool re = !mOutboundUnreliables.empty();

  bool canTick = 
    GetCurrentTimeSeconds() - mLastSendSec > 
    std::max(mOwner->tickSecond(), mTickSec);

  return canTick;
}

bool UDPConnection::tryAppendHeartbeat() {
  if (!mHeartBeatTimer.decrement()) return false;

  NetMessage heartbeat("heartbeat");

  uint64_t time = mOwner->sessionTimeMs();

  heartbeat << time;
  send(heartbeat);

  return true;
}


bool UDPConnection::confirmReceived(uint16_t ack) {


  PacketTracker& pt = packetTracker(ack);
  if (!pt.occupied(ack)) return false;


  // compute rtt and update tracker
  if(cycLessEq(ack, mLastSendAck)) {
    double packetRtt = GetCurrentTimeSeconds() - pt.sendSec;
    // Log::logf("send: %lf, rtt: %lf", tracker.sendSec, packetRtt);
    rtt(packetRtt);
    // Log::logf("[%u]computed rtt: %lf, ack: %u, largestReceivedAck: %u", mIndexOfSession, rtt(), ack, mLargestReceivedAck);
  }

  // confirm reliable message
  auto reliables = pt.reliables();
  for(uint16_t reliable: reliables) {
    // bool confirmed = false;
    for(uint i = (uint)mSentReliable.size() - 1; i < (uint)mSentReliable.size(); i--) {
      if(mSentReliable[i].reliableId() == reliable) {
        mSentReliable[i] = mSentReliable.back();
        mSentReliable.pop_back();
        // confirmed = true;
        break;
      }
    }
    // ENSURES(confirmed);
  }

  pt.reset();

  return true;
}

UDPConnection::PacketTracker& UDPConnection::packetTracker(uint ack) {
  return mTrackers[ack % PACKET_TRACKER_CACHE_SIZE];
}

bool UDPConnection::canSendNewReliable() const {
  uint16_t nextReliable = mNextReliableId;

  if (mSentReliable.empty()) return true;

  uint16_t diff = nextReliable - mOldestUnconfirmedRelialbeId;

  return diff <= RELIALBE_WINDOW_SIZE;
}
