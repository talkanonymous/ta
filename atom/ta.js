var taDefaultIdentityName = "anonymous";
var taIdentityName = "";
var taLastOriginalStatusText = "";
var taListenWaiting = 0;
var taListenInterval;
var taListenIntervalMilliseconds = 4000;
var taListenPhraseCount = 0;
var taListenPhraseCountMax = 16;
var taNextPhraseId = 0;
var taProvisionalTalkCount = 0;
var taStateRepeatCount = 0;
var taStyleSheetTitle;
var taVerbose = 0;

function addPhrase(text, style)
{
  var li = document.createElement('li');
  if (style) {
    li.setAttribute("class", style);
  }
  text = formatPhrase(text, style);
  li.innerHTML = text;
  document.getElementById('listen_ul').appendChild(li);

  taListenPhraseCount++;

  if ("provisional" != style) {
    if (taListenPhraseCount > taListenPhraseCountMax) {
      var firstLi = document.getElementById('listen_ul').childNodes[0];
      document.getElementById('listen_ul').removeChild(firstLi);
    }
  }
}

function be(identityName)
{
  addPhrase("you are <a href='/be/" + identityName + "'>" + identityName
      + "</a>", "system");
  if (identityName != taIdentityName) {

    var pastIdentities = readCookie("identities");
    if (pastIdentities) {
      var pastIdentitiesArray = readCookie("identities").split('\t');
      if (identityName
          != pastIdentitiesArray[pastIdentitiesArray.length - 1]) {
        pastIdentities += "\t" + identityName;
        createCookie("identities", pastIdentities, 0);
      }
    } else {
      pastIdentities = identityName;
      createCookie("identities", pastIdentities, 0);
    }

    taIdentityName = identityName;
    taNextPhraseId = 0;
    listen();
  }
}

function changeStyle()
{
  if ('hipster' == taStyleSheetTitle) {
    setActiveStyleSheet('comic');
  } else if ('comic' == taStyleSheetTitle) {
    setActiveStyleSheet('hacker');
  } else {
    setActiveStyleSheet('hipster');
  }
  resetListen();
}

function decode(str)
{
  return unescape(str.replace(/\+/g, " "));
}

function doCommand(command)
{
  var talk = document.getElementById('talk_input');
  talk.value = command;
  var talk_form = document.getElementById('talk_form');
  talk_form.submit();
  setFocusOnTalk();
}

function formatPhrase(text, style)
{

  if ("hacker" == taStyleSheetTitle) {
    if ("system" == style) {
      text = "<span style='color: #0000cd;'>-</span><span style='color: white;'>!</span><span style='color: #0000cd;'>-</span> " + text;
    } else {
      text = "<span style='color: #4d4d4d;'>&lt;</span> <span style='color: white;'>" + taIdentityName + "</span> <span style='color: #4d4d4d;'>&gt;</span> " + text;
    }
    var d = new Date();
    text = "<span style='color: #dcdcdc;'>" + padDigit(d.getUTCHours()) + ":"
      + padDigit(d.getUTCMinutes()) + ":" + padDigit(d.getUTCSeconds())
      + "</span> " + text;
  }

  return text;
}

function getElementsByClass(element, className)
{
  var itemsfound = new Array;
  var elements = element.getElementsByTagName('*');
  for (var i = 0; i < elements.length; i++) {
    if (elements[i].className == className) {
      itemsfound.push(elements[i]);
    }
  }
  return itemsfound;
}

function hide(divId)
{
  var item = document.getElementById(divId);
  item.className = 'hidden';
}

function init()
{
  state('init');

  var identityName = taDefaultIdentityName;
  var matches = document.location.href.match(/\/be\/(.+)/);
  if (matches) {
    identityName = decode(matches[1]);
    document.title = "talk [?] " + identityName;
  }
  be(identityName);
  setFocusOnTalk();
}

function listen()
{
  if (taListenWaiting) {
    return;
  }

  //state('listen');

  if (taListenInterval) {
    clearInterval(taListenInterval);
    delete taListenInterval;
  }

  taListenWaiting = 1;
  $.get("/listen", {"next_phrase_id" : taNextPhraseId,
          "identity_name" : taIdentityName,
          "force_request" : Math.random()},
      function(data) {
        taListenWaiting = 0;
        var lines = data.split('\n');
        if ('ok_nothing_to_listen_to' == lines[0].split(':')[1]) {
          //console.log('nothing to listen to');
        } else {
          removeProvisionalTalks();
          for (var i = 0; i < lines.length; i++) {
              var line = lines[i];
              if ("" == line) {
                break;
              }
              if ("status:ok_nothing_to_listen_to" == line) {
                break;
              }
              var pairs = line.split('\t');
              var id = pairs[0];
              var text = pairs[1];
              taNextPhraseId = parseInt(id, 10) + parseInt(1, 10);
              text = redact(text);
              addPhrase(text);
            }
          }
      },
      "text");

  taListenInterval = setInterval('listen();', taListenIntervalMilliseconds);
}

function padDigit(digit)
{
  if (digit < 10) {
    digit = '0' + digit;
  }
  return digit;
}

function removeProvisionalTalks()
{
  var list = document.getElementById('listen_ul');
  var listItems = getElementsByClass(list, "provisional");
  var listItemLength = listItems.length;
  for (var i = 0; i < listItemLength; i++) {
    listItem = listItems[i];
    list.removeChild(listItem);
    taProvisionalTalkCount--;
    taListenPhraseCount--;
  }

  if (taProvisionalTalkCount != 0) {
    console.log("provisional talk count is not what was expected");
  }
  taProvisionalTalkCount = 0;
}

function removeChildrenFromNode(e)
{
  if (!e) {
    return false;
  }
  if (typeof(e) == 'string') {
    e = xGetElementById(e);
  }
  while (e.hasChildNodes()) {
    e.removeChild(e.firstChild);
  }
  return true;
}

function resetListen()
{
  var list = document.getElementById('listen_ul');
  removeChildrenFromNode(list);
  taListenPhraseCount = 0;
  taNextPhraseId = 0;
  be(taIdentityName);
  listen();
}

function setFocusOnTalk()
{
  var talk = document.getElementById('talk_input');
  talk.focus();
}

function show(divId)
{
  var item = document.getElementById(divId);
  item.className = 'shown';
}

function state(text)
{
  var status = document.getElementById('status');
  if (taLastOriginalStatusText == text) {
    taStateRepeatCount++;
    status.innerHTML = text + " [x" + taStateRepeatCount + "]";
  } else {
    taStateRepeatCount = 1;
    status.innerHTML = text;
    taLastOriginalStatusText = text;
  }
  if ("hacker" == taStyleSheetTitle) {
    addPhrase(text, "system");
  }
}

function touchLogo()
{
  if (taVerbose) {
    taVerbose = 0;
    hide('help');
  } else {
    taVerbose = 1;
    show('help');
  }
  setFocusOnTalk();
}

function talk()
{
  var talk = document.getElementById('talk_input');

  if (!talk.value) {
    return;
  }

  var matches_style = talk.value.match(/^\/style/);
  var matches_command = talk.value.match(/^\/(.*)/);
  var matches_be_anonymous = talk.value.match(/^\/be/);
  var matches_be = talk.value.match(/^\/be[\s+\/](.*)/);
  var matches_random = talk.value.match(/^\/random/);
  var matches_whoami = talk.value.match(/^\/whoami/);
  var matches_whowasi = talk.value.match(/^\/whowasi/);
  var matches_help = talk.value.match(/^\/help/);
  var matches_help_short = talk.value.match(/^\/\?/);
  if (matches_be) {
    var newIdentity = trim(matches_be[1]);
    if (!newIdentity) {
      newIdentity = taDefaultIdentityName;
    }
    be(newIdentity);
    talk.value = "";
    state('you can be whoever you want');

  } else if (matches_be_anonymous) {
    be(taDefaultIdentityName);
    talk.value = "";
    state('that\'s a good choice');

  } else if (matches_whoami) {
    addPhrase("you are <a href='/be/" + taIdentityName + "'>"
        + taIdentityName + "</a>", "system");
    talk.value = "";
    state('I really can\'t help you with that');

  } else if (matches_whowasi) {
    var i;
    var pastIdentities = readCookie("identities").split("\t");
    var pastIdentityNames = "";
    for (i = 0; i < pastIdentities.length; i++) {
      identityName = pastIdentities[i];
      pastIdentityNames += "<a href='/be/" + identityName + "'>"
        + identityName + "</a> ";
    }
    addPhrase("you were " + pastIdentityNames, "system");
    talk.value = "";
    if (pastIdentities.length < 2) {
      state('try being someone else for a while');
    } else if (pastIdentities.length < 4) {
      state('that\'s not very many to remember');
    } else {
      state('you really get around, don\'t you');
    }

  } else if (matches_help || matches_help_short) {
    touchLogo();
    talk.value = "";
    state('we all need a little help sometimes');

  } else if (matches_random) {
    be(Math.round(Math.random() * 1000000));
    talk.value = "";
    state('I know you are');

  } else if (matches_style) {
    changeStyle();
    talk.value = "";
    state('stylin\' and profilin\'');

  } else if (matches_command) {
    addPhrase(matches_command[1] + ": command not found", "system");
    talk.value = "";
    state('you can\'t just make this shit up');

  } else {
    //state('talk');
    var text = talk.value;
    talk.value = "";
    addPhrase(text, "provisional");
    taProvisionalTalkCount++;
    $.get("/talk", {"text" : text,
            "identity_name" : taIdentityName,
            "force_request" : Math.random()},
        function(data) {
          var lines = data.split('\n');
          var line = lines[0];
          var pairs = line.split(':');
          var status = pairs[1];
          if ("ok" != status) {
            console.log("/talk failed");
          }
        },
        "text");
    listen();

  }
}

function trim(stringToTrim)
{
  return stringToTrim.replace(/^\s+|\s+$/g, "");
}

// --[ stylesheet library ]----------------------------------------------------

function setActiveStyleSheet(title) {
  var i, a, main;
  for(i=0; (a = document.getElementsByTagName("link")[i]); i++) {
    if(a.getAttribute("rel").indexOf("style") != -1 && a.getAttribute("title")) {
      a.disabled = true;
      if(a.getAttribute("title") == title) a.disabled = false;
    }
  }
  taStyleSheetTitle = title;
}

function getActiveStyleSheet() {
  var i, a;
  for(i=0; (a = document.getElementsByTagName("link")[i]); i++) {
    if(a.getAttribute("rel").indexOf("style") != -1 && a.getAttribute("title") && !a.disabled) return a.getAttribute("title");
  }
  return null;
}

function getPreferredStyleSheet() {
  var i, a;
  for(i=0; (a = document.getElementsByTagName("link")[i]); i++) {
    if(a.getAttribute("rel").indexOf("style") != -1
       && a.getAttribute("rel").indexOf("alt") == -1
       && a.getAttribute("title")
       ) return a.getAttribute("title");
  }
  return null;
}

function createCookie(name,value,days) {
  if (days) {
    var date = new Date();
    date.setTime(date.getTime()+(days*24*60*60*1000));
    var expires = "; expires="+date.toGMTString();
  }
  else expires = "";
  document.cookie = name+"="+value+expires+"; path=/";
}

function readCookie(name) {
  var nameEQ = name + "=";
  var ca = document.cookie.split(';');
  for(var i=0;i < ca.length;i++) {
    var c = ca[i];
    while (c.charAt(0)==' ') c = c.substring(1,c.length);
    if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
  }
  return null;
}

window.onload = function(e) {
  var cookie = readCookie("style");
  var title = cookie ? cookie : getPreferredStyleSheet();
  setActiveStyleSheet(title);
}

window.onunload = function(e) {
  var title = getActiveStyleSheet();
  createCookie("style", title, 365);
}

var cookie = readCookie("style");
var title = cookie ? cookie : getPreferredStyleSheet();
if ((title != 'hipster') && (title != 'comic') && (title != 'hacker')) {
  title = 'hipster';
}
setActiveStyleSheet(title);

function createCookie(name,value,days)
{
  if (days) {
    var date = new Date();
    date.setTime(date.getTime()+(days*24*60*60*1000));
    var expires = "; expires="+date.toGMTString();
  }
  else var expires = "";
  document.cookie = name+"="+value+expires+"; path=/";
}

function readCookie(name)
{
  var nameEQ = name + "=";
  var ca = document.cookie.split(';');
  for(var i=0;i < ca.length;i++) {
    var c = ca[i];
    while (c.charAt(0)==' ') c = c.substring(1,c.length);
    if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
  }
  return null;
}

function eraseCookie(name)
{
  createCookie(name,"",-1);
}
