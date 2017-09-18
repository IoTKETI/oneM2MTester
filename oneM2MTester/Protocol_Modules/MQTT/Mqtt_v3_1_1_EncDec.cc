/******************************************************************************
* Copyright (c) 2016  Ericsson AB
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html
*
* Contributors:
*  Mate Kovacs - initial implementation and initial documentation
******************************************************************************/
//
//  File:               Mqtt_v3.1.1_EncDec.cc
//  Rev:                R1A
//  Prodnr:             CNL 113 831

#include "Mqtt_v3_1_1_Types.hh"

namespace Mqtt__v3__1__1__Types {

unsigned int encodeFlags(BIT4n bits);
int encodePacketIdentifier(TTCN_Buffer &buffer, int identifier, long long int &length);
int encodeUtf8(TTCN_Buffer &stream, UNIVERSAL_CHARSTRING str, long long int &length);
int encodeOctetstring(TTCN_Buffer &stream, OCTETSTRING str, long long int &length);
int decodeInteger(const unsigned char* &str, const int position, const int length);

INTEGER
f__MQTT__v3__1__1__dec(OCTETSTRING const &str, MQTT__v3__1__1__Message &msg)
{
  if(TTCN_Logger::log_this_event(TTCN_DEBUG)){
    TTCN_Logger::begin_event(TTCN_DEBUG);
    TTCN_Logger::log_event("Decoding MQTT 3.1.1 message: ");
    str.log();
    TTCN_Logger::end_event();
  }

  if(str.lengthof() >= 2){
    const unsigned char lookup[16] = {
    0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
    0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};
    const unsigned char* str_ptr = (const unsigned char*) str;
    int packetType = str_ptr[0] >> 4;
    unsigned char bitstr = lookup[str_ptr[0] & 15];
    int position = 1;
    int prePosition = 1;
    int count = 0;
    int multiplier = 1;
    int length = 0;
    unsigned char encodedByte;
    int tmpLength;
    unsigned char flag;

    //get packet length
    do{
      if(str.lengthof() < position){
        msg.raw__message() = str;
        return 1;
      }
      encodedByte = str_ptr[position];
      length += (encodedByte & 127) * multiplier;
      multiplier *= 128;
      if(multiplier > 128*128*128){
        msg.raw__message() = str;
        return 1;
      }
      position++;
      prePosition++;
    }while((encodedByte & 128) != 0);

    switch(packetType){
      case 1:
        msg.msg().connect__msg().header().flags() = BITSTRING(4, &bitstr);
        if(str.lengthof() < position+1){
          msg.raw__message() = str;
          return 1;
        }
        tmpLength = decodeInteger(str_ptr, position, 2);
        if(str.lengthof() < position+8+tmpLength){
          msg.raw__message() = str;
          return 1;
        }
        msg.msg().connect__msg().name().decode_utf8(tmpLength, &str_ptr[position+2]);
        position += 2+tmpLength;
        msg.msg().connect__msg().protocol__level() = decodeInteger(str_ptr, position, 1);
        position++;
        //flags
        flag = (str_ptr[position] & 128) >> 7;
        msg.msg().connect__msg().flags().user__name__flag() = BITSTRING(1, &flag);
        flag = (str_ptr[position] & 64) >> 6;
        msg.msg().connect__msg().flags().password__flag() = BITSTRING(1, &flag);
        flag = (str_ptr[position] & 32) >> 5;
        msg.msg().connect__msg().flags().will__retain() = BITSTRING(1, &flag);
        msg.msg().connect__msg().flags().will__qos() = (str_ptr[position] >> 3) & 3;
        flag = (str_ptr[position] & 4) >> 2;
        msg.msg().connect__msg().flags().will__flag() = BITSTRING(1, &flag);
        flag = (str_ptr[position] & 2) >> 1;
        msg.msg().connect__msg().flags().clean__session() = BITSTRING(1, &flag);
        position++;
        msg.msg().connect__msg().keep__alive() = decodeInteger(str_ptr, position, 2);
        position += 2;
        //payload
        tmpLength = decodeInteger(str_ptr, position, 2);
        if(str.lengthof() < position+2+tmpLength){
          msg.raw__message() = str;
          return 1;
        }
        msg.msg().connect__msg().payload().client__identifier().decode_utf8(tmpLength, &str_ptr[position+2]);
        position += 2+tmpLength;
        if(msg.msg().connect__msg().flags().will__flag()[0].get_bit() == 1){
          if(str.lengthof() < position+2){
            msg.raw__message() = str;
            return 1;
          }
          tmpLength = decodeInteger(str_ptr, position, 2);
          if(str.lengthof() < position+4+tmpLength){
            msg.raw__message() = str;
            return 1;
          }
          msg.msg().connect__msg().payload().will__topic()().decode_utf8(tmpLength, &str_ptr[position+2]);
          position += 2+tmpLength;

          tmpLength = decodeInteger(str_ptr, position, 2);
          if(str.lengthof() < position+2+tmpLength){
            msg.raw__message() = str;
            return 1;
          }
          msg.msg().connect__msg().payload().will__message()() = OCTETSTRING(tmpLength, &str_ptr[position+2]);
          position += 2+tmpLength;
        }else{
          msg.msg().connect__msg().payload().will__topic() = OMIT_VALUE;
          msg.msg().connect__msg().payload().will__message() = OMIT_VALUE;
        }
        if(msg.msg().connect__msg().flags().user__name__flag()[0].get_bit() == 1){
          if(str.lengthof() < position+2){
            msg.raw__message() = str;
            return 1;
          }
          tmpLength = decodeInteger(str_ptr, position, 2);
          if(str.lengthof() < position+2+tmpLength){
            msg.raw__message() = str;
            return 1;
          }
          msg.msg().connect__msg().payload().user__name()().decode_utf8(tmpLength, &str_ptr[position+2]);
          position += 2+tmpLength;
        }else{
          msg.msg().connect__msg().payload().user__name() = OMIT_VALUE;
        }
        if(msg.msg().connect__msg().flags().password__flag()[0].get_bit() == 1){
          if(str.lengthof() < position+2){
            msg.raw__message() = str;
            return 1;
          }
          tmpLength = decodeInteger(str_ptr, position, 2);
          if(str.lengthof() < position+2+tmpLength){
            msg.raw__message() = str;
            return 1;
          }
          msg.msg().connect__msg().payload().password()() = OCTETSTRING(tmpLength, &str_ptr[position+2]);
          position += 2+tmpLength;
        }else{
          msg.msg().connect__msg().payload().password() = OMIT_VALUE;
        }
        break;
      case 2:
        msg.msg().connack().header().flags() = BITSTRING(4, &bitstr);
        if(str.lengthof() >= position+2){
          bitstr = str_ptr[position] & 1;
          msg.msg().connack().session__present__flag() = BITSTRING(1, &bitstr);
          msg.msg().connack().connect__return__code() = decodeInteger(str_ptr, position+1, 1);
          position += 2;
        }
        break;
      case 3:
        msg.msg().publish().header().dup__flag() = BITSTRING(1, &bitstr);
        msg.msg().publish().header().qos__level() = lookup[bitstr & 6] >> 1;
        flag = (bitstr >> 3) & 1;
        msg.msg().publish().header().retain__flag() = BITSTRING(1, &flag);
        if(str.lengthof() < position+2){
          msg.raw__message() = str;
          return 1;
        }
        tmpLength = decodeInteger(str_ptr, position, 2);
        if(str.lengthof() < position+2+tmpLength){
          msg.raw__message() = str;
          return 1;
        }
        msg.msg().publish().topic__name().decode_utf8(tmpLength, &str_ptr[position+2]);
        position += 2+tmpLength;
        if((msg.msg().publish().header().qos__level() == Mqtt__v3__1__1__Types::QoS::AT__LEAST__ONCE__DELIVERY || msg.msg().publish().header().qos__level() == Mqtt__v3__1__1__Types::QoS::EXACTLY__ONE__DELIVERY) && str.lengthof() >= position+2){
          if(str.lengthof() < position+2){
            msg.raw__message() = str;
            return 1;
          }
          msg.msg().publish().packet__identifier()() = decodeInteger(str_ptr, position, 2);
          position += 2;
        }else{
          msg.msg().publish().packet__identifier() = OMIT_VALUE;
        }
        msg.msg().publish().payload() = OCTETSTRING(length - position + prePosition, &str_ptr[position]);
        position += (length - position + prePosition);
        break;
      case 4:
        msg.msg().puback().header().flags() = BITSTRING(4, &bitstr);
        if(str.lengthof() >= position+2){
          msg.msg().puback().packet__identifier() = decodeInteger(str_ptr, position, 2);
          position += 2;
        }else{
          msg.raw__message() = str;
          return 1;
        }
        break;
      case 5:
        msg.msg().pubrec().header().flags() = BITSTRING(4, &bitstr);
        if(str.lengthof() >= position+2){
          msg.msg().pubrec().packet__identifier() = decodeInteger(str_ptr, position, 2);
          position += 2;
        }else{
          msg.raw__message() = str;
          return 1;
        }
        break;
      case 6:
        msg.msg().pubrel().header().flags() = BITSTRING(4, &bitstr);
        if(str.lengthof() >= position+2){
          msg.msg().pubrel().packet__identifier() = decodeInteger(str_ptr, position, 2);
          position += 2;
        }else{
          msg.raw__message() = str;
          return 1;
        }
        break;
      case 7:
        msg.msg().pubcomp().header().flags() = BITSTRING(4, &bitstr);
        if(str.lengthof() >= position+2){
          msg.msg().pubcomp().packet__identifier() = decodeInteger(str_ptr, position, 2);
          position += 2;
        }else{
          msg.raw__message() = str;
          return 1;
        }
        break;
      case 8:
        msg.msg().subscribe().header().flags() = BITSTRING(4, &bitstr);
        if(str.lengthof() >= position+2){
          msg.msg().subscribe().packet__identifier() = decodeInteger(str_ptr, position, 2);
          position += 2;
        }else{
          msg.raw__message() = str;
          return 1;
        }
        while(str.lengthof() > position){
          if(str.lengthof() < position+2){
            msg.raw__message() = str;
            return 1;
          }
          tmpLength = decodeInteger(str_ptr, position, 2);
          if(str.lengthof() < position+3+tmpLength){
            msg.raw__message() = str;
            return 1;
          }
          msg.msg().subscribe().payload()[count].topic__filter().decode_utf8(tmpLength, &str_ptr[position+2]);
          msg.msg().subscribe().payload()[count].requested__qos() = str_ptr[position+tmpLength+2];
          count++;
          position += 3+tmpLength;
        }
        if(count == 0){
          msg.msg().subscribe().payload() = Mqtt__v3__1__1__Types::MQTT__v3__1__1__SubscribePayloadList(NULL_VALUE);
        }
        break;
      case 9:
        msg.msg().suback().header().flags() = BITSTRING(4, &bitstr);
        if(str.lengthof() >= position+2){
          msg.msg().suback().packet__identifier() = decodeInteger(str_ptr, position, 2);
          position += 2;
        }else{
          msg.raw__message() = str;
          return 1;
        }
        while(str.lengthof() > position){
          if(str.lengthof() < position+1){
            msg.raw__message() = str;
            return 1;
          }
          msg.msg().suback().payload().return__code()[count] = decodeInteger(str_ptr, position, 1);
          count++;
          position++;
        }
        if(count == 0){
          msg.msg().suback().payload().return__code() = Mqtt__v3__1__1__Types::IntegerList(NULL_VALUE);
        }
        break;
      case 10:
        msg.msg().unsubscribe().header().flags() = BITSTRING(4, &bitstr);
        if(str.lengthof() >= position+2){
          msg.msg().unsubscribe().packet__identifier() = decodeInteger(str_ptr, position, 2);
          position += 2;
        }else{
          msg.raw__message() = str;
          return 1;
        }
        while(str.lengthof() > position){
          if(str.lengthof() < position+2){
            msg.raw__message() = str;
            return 1;
          }
          tmpLength = decodeInteger(str_ptr, position, 2);
          if(str.lengthof() < position+2+tmpLength){
            msg.raw__message() = str;
            return 1;
          }
          msg.msg().unsubscribe().payload().topic__filter()[count].decode_utf8(tmpLength, &str_ptr[position+2]);
          position += 2+tmpLength;
          count++;
        }
        if(count == 0){
          msg.msg().unsubscribe().payload().topic__filter() = Mqtt__v3__1__1__Types::UCHAR0__65535List(NULL_VALUE);
        }
        break;
      case 11:
        msg.msg().unsuback().header().flags() = BITSTRING(4, &bitstr);
        if(str.lengthof() >= position+2){
          msg.msg().unsuback().packet__identifier() = decodeInteger(str_ptr, position, 2);
          position += 2;
        }else{
          msg.raw__message() = str;
          return 1;
        }
        break;
      case 12:
        msg.msg().pingreq().header().flags() = BITSTRING(4, &bitstr);
        break;
      case 13:
        msg.msg().pingresp().header().flags() = BITSTRING(4, &bitstr);
        break;
      case 14:
        msg.msg().disconnect__msg().header().flags() = BITSTRING(4, &bitstr);
        break;
      case 0:
      case 15:
      default:
        msg.raw__message() = str;
        return 0;
        break;
    }
    if(str.lengthof() > position or length != (position-prePosition)){
      msg.raw__message() = str;
      return 1;
    }
  }else{
    msg.raw__message() = str;
    return 1;
  }

  if(TTCN_Logger::log_this_event(TTCN_DEBUG)){
    TTCN_Logger::begin_event(TTCN_DEBUG);
    TTCN_Logger::log_event("Decoded MQTT 3.1.1 message: ");
    msg.log();
    TTCN_Logger::end_event();
  }

  return 0;
}

INTEGER
f__MQTT__v3__1__1__enc(MQTT__v3__1__1__Message const &msg, OCTETSTRING &str)
{
  TTCN_Buffer stream_beginning;
  TTCN_Buffer stream;
  unsigned char chr = 0;
  long long int length = 0;

  if(TTCN_Logger::log_this_event(TTCN_DEBUG)){
    TTCN_Logger::begin_event(TTCN_DEBUG);
    TTCN_Logger::log_event("Encoding MQTT 3.1.1 message: ");
    msg.log();
    TTCN_Logger::end_event();
  }

  switch(msg.get_selection()){
    case MQTT__v3__1__1__Message::ALT_raw__message:
  	  stream.put_os(msg.raw__message());
  	  stream.get_string(str);
  	  return 0;
  	  break;
    case MQTT__v3__1__1__Message::ALT_msg:
    default:
  	  break;
  }

  switch(msg.msg().get_selection()){
    case MQTT__v3__1__1__ReqResp::ALT_connect__msg:
      chr = 1 << 4;
      if(msg.msg().connect__msg().header().flags().lengthof() == 4){
        chr += encodeFlags(msg.msg().connect__msg().header().flags());
        stream_beginning.put_c(chr);
      }else{
        return 1;
      }
      //variable header
      if(encodeUtf8(stream, msg.msg().connect__msg().name(), length) != 0){
        return 1;
      }
      if(0 <= msg.msg().connect__msg().protocol__level() && msg.msg().connect__msg().protocol__level() <= 255){
        chr = (int) msg.msg().connect__msg().protocol__level();
        stream.put_c(chr);
        length++;
      }else{
        return 1;
      }
        //flags
      if(msg.msg().connect__msg().flags().user__name__flag().lengthof() == 1 && msg.msg().connect__msg().flags().password__flag().lengthof() == 1 &&
       msg.msg().connect__msg().flags().will__retain().lengthof() == 1 && msg.msg().connect__msg().flags().will__flag().lengthof() == 1 &&
       msg.msg().connect__msg().flags().clean__session().lengthof() == 1){
        chr = msg.msg().connect__msg().flags().user__name__flag()[0].get_bit() << 7;
        chr += msg.msg().connect__msg().flags().password__flag()[0].get_bit() << 6;
        chr += msg.msg().connect__msg().flags().will__retain()[0].get_bit() << 5;
        chr += ((int) msg.msg().connect__msg().flags().will__qos()) << 3;
        chr += msg.msg().connect__msg().flags().will__flag()[0].get_bit() << 2;
        chr += msg.msg().connect__msg().flags().clean__session()[0].get_bit() << 1;
        stream.put_c(chr);
        length++;
      }else{
        return 1;
      }
      if(0 <= msg.msg().connect__msg().keep__alive() && msg.msg().connect__msg().keep__alive() <= 65535){
        chr = (int) msg.msg().connect__msg().keep__alive() >> 8;
        stream.put_c(chr);
        chr = (int) msg.msg().connect__msg().keep__alive();
        stream.put_c(chr);
        length += 2;
      }else{
        return 1;
      }
      //payload
      if(encodeUtf8(stream, msg.msg().connect__msg().payload().client__identifier(), length) != 0){
        return 1;
      }
      if(msg.msg().connect__msg().payload().will__topic().ispresent()){
        if(encodeUtf8(stream, msg.msg().connect__msg().payload().will__topic()(), length) != 0){
          return 1;
        }
      }
      if(msg.msg().connect__msg().payload().will__message().ispresent()){
        if(encodeOctetstring(stream, msg.msg().connect__msg().payload().will__message()(), length) != 0){
          return 1;
        }
      }
      if(msg.msg().connect__msg().payload().user__name().ispresent()){
        if(encodeUtf8(stream, msg.msg().connect__msg().payload().user__name()(), length) != 0){
          return 1;
        }
      }
      if(msg.msg().connect__msg().payload().password().ispresent()){
        if(encodeOctetstring(stream, msg.msg().connect__msg().payload().password()(), length) != 0){
          return 1;
        }
      }
      break;
    case MQTT__v3__1__1__ReqResp::ALT_connack:
      chr = 2 << 4;
      if(msg.msg().connack().header().flags().lengthof() == 4){
        chr += encodeFlags(msg.msg().connack().header().flags());
        stream_beginning.put_c(chr);
      }else{
        return 1;
      }
      //variable header
      if(msg.msg().connack().session__present__flag().lengthof() == 1){
        chr = msg.msg().connack().session__present__flag()[0].get_bit();
        stream.put_c(chr);
        length++;
      }else{
        return 1;
      }
      if((int) msg.msg().connack().connect__return__code() < 0 || 255 < (int) msg.msg().connack().connect__return__code()){
        return 1;
      }
      chr = (int) msg.msg().connack().connect__return__code();
      stream.put_c(chr);
      length++;
      break;
    case MQTT__v3__1__1__ReqResp::ALT_publish:
      const unsigned char* tmp;
      chr = 3 << 4;
      if(msg.msg().publish().header().dup__flag().lengthof() == 1 && msg.msg().publish().header().retain__flag().lengthof() == 1){
        tmp = (const unsigned char*) msg.msg().publish().header().dup__flag();
        chr += tmp[0] << 3;
        chr += msg.msg().publish().header().qos__level() << 1;
        tmp = (const unsigned char*) msg.msg().publish().header().retain__flag();
        chr += tmp[0];
      }else{
        return 1;
      }
      stream_beginning.put_c(chr);
      //variable header
      if(encodeUtf8(stream, msg.msg().publish().topic__name(), length) != 0){
        return 1;
      }
      if(msg.msg().publish().packet__identifier().is_present()){
        if(encodePacketIdentifier(stream, (int) msg.msg().publish().packet__identifier()(), length) != 0){
          return 1;
        }
      }
      //payload
      stream.put_os(msg.msg().publish().payload());
      length += msg.msg().publish().payload().lengthof();
      break;
    case MQTT__v3__1__1__ReqResp::ALT_puback:
      chr = 4 << 4;
      if(msg.msg().puback().header().flags().lengthof() == 4){
        chr += encodeFlags(msg.msg().puback().header().flags());
        stream_beginning.put_c(chr);
      }else{
        return 1;
      }
      //variable header
      if(encodePacketIdentifier(stream, (int) msg.msg().puback().packet__identifier(), length) != 0){
        return 1;
      }
      break;
    case MQTT__v3__1__1__ReqResp::ALT_pubrec:
      chr = 5 << 4;
      if(msg.msg().pubrec().header().flags().lengthof() == 4){
        chr += encodeFlags(msg.msg().pubrec().header().flags());
        stream_beginning.put_c(chr);
      }else{
        return 1;
      }

      //variable header
      if(encodePacketIdentifier(stream, (int) msg.msg().pubrec().packet__identifier(), length) != 0){
        return 1;
      }
      break;
    case MQTT__v3__1__1__ReqResp::ALT_pubrel:
      chr = 6 << 4;
      if(msg.msg().pubrel().header().flags().lengthof() == 4){
        chr += encodeFlags(msg.msg().pubrel().header().flags());
        stream_beginning.put_c(chr);
      }else{
        return 1;
      }
      //variable header
      if(encodePacketIdentifier(stream, (int) msg.msg().pubrel().packet__identifier(), length) != 0){
        return 1;
      }
      break;
    case MQTT__v3__1__1__ReqResp::ALT_pubcomp:
      chr = 7 << 4;
      if(msg.msg().pubcomp().header().flags().lengthof() == 4){
        chr += encodeFlags(msg.msg().pubcomp().header().flags());
        stream_beginning.put_c(chr);
      }else{
        return 1;
      }
      //variable header
      if(encodePacketIdentifier(stream, (int) msg.msg().pubcomp().packet__identifier(), length) != 0){
        return 1;
      }
      break;
    case MQTT__v3__1__1__ReqResp::ALT_subscribe:
      chr = 8 << 4;
      if(msg.msg().subscribe().header().flags().lengthof() == 4){
        chr += encodeFlags(msg.msg().subscribe().header().flags());
        stream_beginning.put_c(chr);
      }else{
        return 1;
      }
      //variable header
      if(encodePacketIdentifier(stream, (int) msg.msg().subscribe().packet__identifier(), length) != 0){
        return 1;
      }
      //payload
      for(int i = 0; i < msg.msg().subscribe().payload().size_of(); i++){
        if(encodeUtf8(stream, msg.msg().subscribe().payload()[i].topic__filter(), length) != 0){
          return 1;
        }
        chr = msg.msg().subscribe().payload()[i].requested__qos();
        stream.put_c(chr);
        length++;
      }
      break;
    case MQTT__v3__1__1__ReqResp::ALT_suback:
      chr = 9 << 4;
      if(msg.msg().suback().header().flags().lengthof() == 4){
        chr += encodeFlags(msg.msg().suback().header().flags());
        stream_beginning.put_c(chr);
      }else{
        return 1;
      }
      //variable header
      if(encodePacketIdentifier(stream, (int) msg.msg().suback().packet__identifier(), length) != 0){
        return 1;
      }
      //payload
      for(int i = 0; i < msg.msg().suback().payload().return__code().size_of(); i++){
        if(msg.msg().suback().payload().return__code()[i] < 0 || 255 < msg.msg().suback().payload().return__code()[i]){
          return 1;
        }
        chr = msg.msg().suback().payload().return__code()[i];
        stream.put_c(chr);
        length++;
      }
      break;
    case MQTT__v3__1__1__ReqResp::ALT_unsubscribe:
      chr = 10 << 4;
      if(msg.msg().unsubscribe().header().flags().lengthof() == 4){
        chr += encodeFlags(msg.msg().unsubscribe().header().flags());
        stream_beginning.put_c(chr);
      }else{
        return 1;
      }
      //variable header
      if(encodePacketIdentifier(stream, (int) msg.msg().unsubscribe().packet__identifier(), length) != 0){
        return 1;
      }
      //payload
      for(int i = 0; i < msg.msg().unsubscribe().payload().topic__filter().size_of(); i++){
        if(encodeUtf8(stream, msg.msg().unsubscribe().payload().topic__filter()[i], length) != 0){
          return 1;
        }
      }
      break;
    case MQTT__v3__1__1__ReqResp::ALT_unsuback:
      chr = 11 << 4;
      if(msg.msg().unsuback().header().flags().lengthof() == 4){
        chr += encodeFlags(msg.msg().unsuback().header().flags());
        stream_beginning.put_c(chr);
      }else{
        return 1;
      }
      //variable header
      if(encodePacketIdentifier(stream, (int) msg.msg().unsuback().packet__identifier(), length) != 0){
        return 1;
      }
      break;
    case MQTT__v3__1__1__ReqResp::ALT_pingreq:
      chr = 12 << 4;
      if(msg.msg().pingreq().header().flags().lengthof() == 4){
        chr += encodeFlags(msg.msg().pingreq().header().flags());
        stream_beginning.put_c(chr);
      }else{
        return 1;
      }
      break;
    case MQTT__v3__1__1__ReqResp::ALT_pingresp:
      chr = 13 << 4;
      if(msg.msg().pingresp().header().flags().lengthof() == 4){
        chr += encodeFlags(msg.msg().pingresp().header().flags());
        stream_beginning.put_c(chr);
      }else{
        return 1;
      }
      break;
    case MQTT__v3__1__1__ReqResp::ALT_disconnect__msg:
      chr = 14 << 4;
      if(msg.msg().disconnect__msg().header().flags().lengthof() == 4){
        chr += encodeFlags(msg.msg().disconnect__msg().header().flags());
        stream_beginning.put_c(chr);
      }else{
        return 1;
      }
      break;
    default:
      return 1;
      break;
  }

  TTCN_Logger::begin_event(TTCN_DEBUG);
  TTCN_Logger::log_event("Encoded MQTT 3.1.1 message: ");
  stream_beginning.log();
  TTCN_Logger::end_event();

  //set packet length
  do{
    unsigned char encodedByte = length % 128;
    length /= 128;
    if(length > 0){
      encodedByte = encodedByte | 128;
    }
    stream_beginning.put_c(encodedByte);
  }while(length > 0);
  stream_beginning.put_buf(stream);
  stream_beginning.get_string(str);

  return 0;
}

unsigned int encodeFlags(BIT4n bits){
  const unsigned char* chr;
  const unsigned char lookup[16] = {
    0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
    0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};

  chr = (const unsigned char*) bits;
  return lookup[chr[0]];
}

int encodePacketIdentifier(TTCN_Buffer &buffer, int identifier, long long int &length){
  if(0 <= identifier && identifier <= 65535){
    unsigned char chr;
    chr = (identifier >> 8) & 255;
    buffer.put_c(chr);
    chr = identifier & 255;
    buffer.put_c(chr);
    length += 2;
    return 0;
  }else{
    return 1;
  }
}

int encodeUtf8(TTCN_Buffer &stream, UNIVERSAL_CHARSTRING str, long long int &length){
  if(0 <= str.lengthof() && str.lengthof() <= 65535){
    unsigned char chr = str.lengthof() >> 8;
    stream.put_c(chr);
    chr = str.lengthof();
    stream.put_c(chr);
    str.encode_utf8(stream, false);
    length += 2 + str.lengthof();
    return 0;
  }else{
    return 1;
  }
}

int encodeOctetstring(TTCN_Buffer &stream, OCTETSTRING str, long long int &length){
  if(0 <= str.lengthof() && str.lengthof() <= 65535){
    unsigned char chr = str.lengthof() >> 8;
    stream.put_c(chr);
    chr = str.lengthof();
    stream.put_c(chr);
    stream.put_os(str);
    length += 2 + str.lengthof();
    return 0;
  }else{
    return 1;
  }
}

int decodeInteger(const unsigned char* &str, const int position, const int length){
  int value = 0;

  for(int i = 0; i < length; i++){
	  value += (str[position+i] << (8 * (length-i-1)));
  }
  return value;
}

}
