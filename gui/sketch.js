// watcher stuff up here
//
function preload() {
  let basePath = "/projects/harduino/gui/";
  loadScript(basePath + "HSlider.js");
  loadScript(basePath + "ToggleSwitch.js");
  loadScript(basePath + "LED.js");
  loadScript(basePath + "BarGraph.js");
  loadScript(basePath + "SignalScope.js");
  loadScript(basePath + "../watcher.js");
}

function howMany(str, arr) {
  let count = 0;
  for(let n = 0; n < arr.length; ++n) {
    if(arr[n].name.search(str) == 0)
      count++;
  }
  return count;
}

let loopbackDemo = false;
let digitalMask = 0;
function controlCallback(data) {
  if(!data.watcher)
    return;
  console.log(data.watcher);
  if(data.watcher.watchers) {
    loopbackDemo = false; // now we are being serious
    let watchers = data.watcher.watchers;
    Watcher.processList(watchers); // so that parseBuffers() works
    nGpios = howMany("digital", watchers) ? 16 : 0;
    nAnalogIn = howMany("analogIn", watchers);
    nAnalogOut = howMany("analogOut", watchers);
    nAudioIn = howMany("audioIn", watchers);
    nAudioOut = howMany("audioOut", watchers);
    setupGuis(false);

    let monitorWatchers = Array();
    let monitorPeriods = Array();
    let streamWatchers = Array();
    for(let n = 0; n < watchers.length; ++n) {
      let w = watchers[n].name;
      if(0 == w.search(/^audio[IO]/)) {
        streamWatchers.push(w);
      } else {
        // scatter updates over time so to minimise chances of dropped blocks.
        // TODO: once the backend becomes less resource-intensive when sending
        // data, remove this.
        monitorWatchers.push(w);
        monitorPeriods.push(3000 + monitorWatchers.length * 64);
      }
    }
    Watcher.sendCommand({
      cmd: "monitor",
      watchers: monitorWatchers,
      periods: monitorPeriods,
    });
    Watcher.sendCommand({
      cmd: "watch",
      watchers: streamWatchers,
    });
  } else console.log(data.watcher);
}

function processValues() {
  let inBufs = Bela.data.buffers;
  if(!inBufs.length)
    return;
  let buffers = Watcher.parseBuffers(inBufs);
  for(let n = 0; n < buffers.length; ++n) {
    let buf = buffers[n];
    let w = buf.watcher;
    let group;
    let num;
    if("digital" == w) {
      group = "digital";
    } else {
      let numStarts = w.search(/[0-9]/);
      if(-1 == numStarts) {
        console.log("Unexpected watcher", w);
        continue;
      }
      group = w.substr(0, numStarts);
      num = parseInt(w.substr(numStarts, w.length));
    }
    let value = buf.buf[0];
    switch(group) {
      case "digital":
        for(let c = 0; c < nGpios && c < leds.length && c < switches.length; ++c) {
          let oldIsIn = switches[c].getState(c);
          let isIn = !!(value & (1 << c));
          let state = !!(value & (1 << (c + nGpios)));
          if(oldIsIn != isIn) {
            switches[c].setState(isIn);
            // force updating the LED hue depending on the switch's state
            // TODO: factor this out from mousePressed() and call that instead of
            // duplicating code here
            leds[c].makeClickable(!isIn);
            if(isIn)
                leds[c].setColour(ledHue);
            else
                leds[c].setColour(buttonHue);
          }
          leds[c].setState(state);
        }
        break;
      case "analogIn":
        if(num < analogLedBars.length) {
          analogLedBars[num].setBarValue(value);
        }
      case "analogOut":
        break;
      case "envIn":
        if(num < audioLedBars.length) {
          audioLedBars[num].setBarValue(value);
        }
        break;
      case "audioIn":
        if(num < scopes.length) {
          for(let n = 0; n < scopes[num].w && n < buf.buf.length; ++n) {
            let sample = buf.buf[n] * 0.5 + 0.5;
            scopes[num].addSample(sample);
          }
        }
        break;
      case "envOut":
        break;
      case "audioOut":
        break;
    }
  }
}
// mostly original code below
function mm2px(mm)
{
  return 96 * mm / 25.4;
}

var ledHue = 400;
var buttonHue = 250;
var analogLedBarHue = 100;
var audioLedBarHue = 200;

var nGpios = 16;
var nAnalogOut = 8;
var nAnalogIn = 8;
var nAudioIn = 2;

var leds = [];
var switches = [];
var ctlSwitches = [];
var sliders = [];
var analogLedBars = [];
var scopes = [];
var audioLedBars = [];

var text_y = 50;
var led_y = 65;
var analog_y = 260;
var analog_in_x = 200;
var led_spacing = 50

function setup() {
  frameRate(20);
  Watcher.sendCommand({cmd: "list"});
  Bela.control.registerCallback("controlCallback", controlCallback);
  createCanvas(windowWidth, 1.5 * windowHeight);
  // setupGuis() once with default counts, for prototyping purposes
  // will be overridden in controlCallback in response to "list"c
  loopbackDemo = true;
  setupGuis();
}
  
function setupGuis(){
  let led_initial_x = windowWidth/2 - (nGpios + 1) * 0.5 * led_spacing;
  
  ctlSwitches = [];
  switches = [];
  leds = [];
  for (let i = 0; i < nGpios; i++)
  {
        let x_position = led_initial_x + (i+1) * 50; 
        
        leds.push(new LED(ledHue, mm2px(10)));

        leds[i].position(x_position, led_y, false);
    
        switches.push(new ToggleSwitch(mm2px(8)));
        switches[i].position(x_position, led_y + 1.5 * leds[i].diameter, false);
        ctlSwitches.push(new ToggleSwitch(mm2px(4), ' ', 'C'));
        ctlSwitches[i].position(x_position, led_y + 3.0 * leds[i].diameter, false);
        ctlSwitches[i].setState(0);
  }
  
  sliders = [];
  let slider_initial_x = 3*windowWidth/4;
  for (let i = 0; i < nAnalogOut; i++)
  {
      sliders.push(new HSlider(mm2px(75), mm2px(8)));
      sliders[i].position(slider_initial_x, analog_y + i * 50, false);
  }

  analogLedBars = [];
  let bar_initial_x = windowWidth/4;
  for (let i = 0; i < nAnalogIn; i++)
  {
      // analogLedBars.push(new BarGraph(mm2px(4), mm2px(8), 10, analogLedBarHue));
      analogLedBars.push(new BarGraph(mm2px(0.25), mm2px(8), 320, analogLedBarHue));

      analogLedBars[i].setEdgeSpacing(5);
      analogLedBars[i].setLedSpacing(0);
      analogLedBars[i].setEdgeRounding(0);
 
      analogLedBars[i].position(bar_initial_x, analog_y + i * 50, false);

  }
  
  scopes = [];
  audioLedBars = [];
  for (let i = 0; i < nAudioIn; i++)
  {
      let pos_x = bar_initial_x  + i * (slider_initial_x - bar_initial_x)
      scopes.push(new SignalScope(mm2px(75), (mm2px(50))));
      scopes[i].position(pos_x, analog_y + (nAnalogIn+2.5) * 50, false);
    
      audioLedBars.push(new BarGraph(mm2px(3.75), mm2px(8), 15, audioLedBarHue));
      audioLedBars[i].position(pos_x, scopes[i].y + 0.5 * scopes[i].h + 30, false);

  }
}

function draw() {
  if(!loopbackDemo)
    processValues();
  background(220);
  
  for (let i = 0; i < leds.length; i++)
    leds[i].draw();
  
  for (let i = 0; i < switches.length; i++)
    switches[i].draw();
  
  for (let i = 0; i < ctlSwitches.length; i++)
    ctlSwitches[i].draw();

  for (let i = 0; i < sliders.length; i++)
    sliders[i].draw();
  
  for (let i = 0; i < sliders.length; i++)
  {
    if(i < analogLedBars.length && loopbackDemo)
    {
      let val = sliders[i].getValue();
      analogLedBars[i].setBarValue(val);
    }
  }
 
  for (let i = 0; i < analogLedBars.length; i++)
    analogLedBars[i].draw();
  
  for (let i = 0; i < scopes.length; i++)
  {
    if(i < sliders.length && loopbackDemo)
      scopes[i].addSample(sliders[i].getValue());
    scopes[i].draw();
  }
  
  for (let i = 0; i < audioLedBars.length; i++)
    audioLedBars[i] .draw();


  /* LABELS */
  push();
  textAlign(CENTER, CENTER);
  textSize(15);
  
  let text_x = windowWidth / 2;//(leds[leds.length-1].x - leds[0].x) * 0.5;
  let text_y = leds[0].y - 40
  let line_y = text_y + 10

  text("DIGITAL I/O", text_x, text_y);
  line(leds[0].x - 0.5 * leds[0].diameter, line_y, leds[leds.length-1].x + 0.5 * leds[leds.length-1].diameter, line_y);
  
  text_x = analogLedBars[0].x
  text_y = analogLedBars[0].y - 40
  line_y = text_y + 10
  
  text("ANALOG IN", text_x, text_y);
  line(analogLedBars[0].x - 0.5 * analogLedBars[0].bar.w, line_y, analogLedBars[0].x + 0.5 * analogLedBars[0].bar.w, line_y)
  
if(sliders.length)
{
  text_x = sliders[0].x
  text_y = sliders[0].y - 40
  line_y = text_y + 10
  
  text("ANALOG OUT", text_x, text_y);
  line(sliders[0].x - 0.5 * sliders[0].w, line_y, sliders[0].x + 0.5 * sliders[0].w, line_y)
}

  
  text_x = (scopes[scopes.length - 1].x -  scopes[0].x)
  text_y = scopes[0].y - 0.5 * scopes[0].h - 40
  line_y = text_y + 10
  
  text("AUDIO IN", text_x, text_y);
  line(scopes[0].x - 0.5 * scopes[0].w, line_y, scopes[scopes.length - 1].x + 0.5 * scopes[scopes.length - 1].w, line_y)

  pop();
  
  /* NUMERIC LABELS */
  
  for (let i = 0; i < leds.length; i++)
  {
     // Labels
    push();
    textAlign(CENTER, CENTER);
    textSize(15);
    stroke(220);
    fill(220);
    text(str(i), leds[i].x, led_y);
    pop();
  }
  
  for (let i = 0; i < sliders.length; i++)
  {
    // Labels
    push();
    textAlign(CENTER, CENTER);
    textSize(15);
    text(str(i), sliders[i].x - 0.5 * sliders[i].w - 25, sliders[i].y, 25);
    pop();
  }
   
  for (let i = 0; i < analogLedBars.length; i++)
  {
    // Labels
    push();
    textAlign(CENTER, CENTER);
    textSize(15);
    text(str(i), analogLedBars[i].x - 0.5 * analogLedBars[i].bar.w - 25, analogLedBars[i].y, 25);
    pop();
  }   
  
  for (let i = 0; i < scopes.length; i++)
  {
    push();
    textAlign(CENTER, CENTER);
    textSize(15);
    text(str(i), scopes[i].x - 0.5 * scopes[i].w - 25, scopes[i].y, 25);
    pop();
  }
  
  for (let i = 0; i < audioLedBars.length; i++)
  {
    push();
    textAlign(CENTER, CENTER);
    textSize(15);
    text(str(i), audioLedBars[i].x - 0.5 * audioLedBars[i].bar.w - 25, audioLedBars[i].y, 25);
    pop();
  }
}

function mousePressed()
{
  for (let i = 0; i < leds.length; i++)
  {
    switches[i].click();
    if(switches[i].hasChanged())
      print("Switch " + i + " has changed!");
    let s_state = switches[i].getState();
    leds[i].makeClickable(!s_state);
    if(!s_state)
        leds[i].setColour(buttonHue);
    else
        leds[i].setColour(ledHue);
    leds[i].click()
    
    if(leds[i].hasChanged())
      print("Led " + i + " has changed!");
  }
  
  for (let i = 0; i < sliders.length; i++)
  {
    sliders[i].click();
  }
  
  for (let i = 0; i < scopes.length; i++)
  {
    scopes[i].click();
  }

}

function mouseReleased()
{
  for (let i = 0; i < sliders.length; i++)
  {
      sliders[i].release();
  }
}

function doubleClicked()
{
  for (let i = 0; i < sliders.length; i++)
  {
    sliders[i].doubleClick();
  }
}



