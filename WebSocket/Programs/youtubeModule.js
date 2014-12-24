// ==UserScript==
// @name        PalmOS - Youtube
// @namespace   PalmOS
// @description PalmOS module for Youtube player
// @include     http://www.youtube.com/*
// @version     1
// @grant       none
// ==/UserScript==

if (window.top != window.self) return;
/// PalmOS.js
function wsEncodeMsg(opcode, data, encodingType = '2') // encoding to packet msg
{
  var toRet = (encodingType + opcode + '|' + data);
  return toRet;
}
function translate_ws_to_s2(ws) // for encoding '2' data to be send ([UNI]abcd -> [2 UNICODE BYTE]_a_b_c_d)
{
  var encoded = new String();
  for (var i = 0; i < ws.length; i++) {
    encoded += String.fromCharCode(Math.floor((ws[i].charCodeAt()) / 256));
    encoded += String.fromCharCode(ws[i].charCodeAt() % 256);
  }
  return encoded;
}
function translate_s2_to_ws(str) // for decoding '2' back to ws (_a_b_c[2 UNICODE BYTE] -> abc[UNI])
{
  var parsed = new String();
  for (var i = 0; i < str.length; i += 2) {
    if (i == str.length - 1) {
      parsed += String.fromCharCode(i);
      break;
    }
    var code = (str[i].charCodeAt() * 256) + (str[i + 1].charCodeAt());
    parsed += String.fromCharCode(code);
  }
  return parsed;
}
function translate_s_to_bytes(str, bArr, startInd)
{
  for (var i = 0; i < str.length; i++) {
    bArr[startInd + i] = str[i].charCodeAt();
  }
}
function wsDecodeMsg(msg) // decode packet msg to data & auto decoding data with ENC
{
  if (msg instanceof Blob) { // binary message
    return {
      error: 'BLOB'
    }
  }
  var encType = msg[0];
  var splitter = msg.indexOf('|');
  if (splitter == - 1) return {
    error: 'INVALID'
  };
  var opcode = msg.substring(1, splitter);
  var dataS = msg.substring(splitter + 1);
  // decoding data
  if (encType == '2') dataS = translate_s2_to_ws(dataS);
  return {
    op: opcode,
    data: dataS,
  };
}
function wsDecodeBlobMsg(msg, callback) // reflow the message for blob loading & reading
{
  var reader = new FileReader();
  reader.addEventListener('loadend', function () {
    var arr = new Uint8Array(reader.result);
    // convert arr to string 
    var str = new String();
    for (var i = 0; i < arr.length; i++) {
      str += String.fromCharCode(arr[i]);
    }
    console.log('DecodeBlob : ' + str.substring(0, 60));
    if (str.indexOf('|') == - 1) { // error , invalid PalmOS packet
      callback({
        error: 'INVALID'
      });
      return;
    }
    console.log(str);
    var packetObj = wsDecodeMsg(str);
    packetObj['dataArr'] = arr.subarray(str.indexOf('|') + 1, arr.length);
    callback(packetObj);
  });
  reader.readAsArrayBuffer(msg);
}
/// Local script

window.addEventListener('load', LocalMain, false);
var connection = new WebSocket('ws://127.0.0.1');
var moviePlayer = document.getElementById('movie_player');
connection.onopen = function () {
    connection.send(wsEncodeMsg('REGIS', 'youtubeModule', '1'));
  };
  // Log errors
  connection.onerror = function (error) {
    console.log('WebSocket Error ' + error);
  };
  // Log messages from the server
  connection.onmessage = function (e) {
    var packet = wsDecodeMsg(e.data);
    console.log("GM recieve : " + packet.op + " , " + packet.data);
    if(packet.op == "RELAY") {
      var data = packet.data.split("|");
      if(data[0] == "YOUT_LOAD") {
        window.location.href = "http://www.youtube.com/watch?v="+data[1];
        console.log("YOUT_LOAD:"+data[1]);
      }
      else if(data[0] == "YOUT_PAUSE") {
        moviePlayer.pauseVideo();
      }
      else if(data[0] == "YOUT_PLAY") {
        moviePlayer.playVideo();
      }
    }
    
  }
function LocalMain()
{
  
}