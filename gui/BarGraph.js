class BarGraph
{
  constructor(ledWidth, ledHeight, nLeds, ledHue, horizontal = true)
  {
    this.led =
    {
      w: ledWidth,
      h: ledHeight
    };
    this.nLeds = nLeds;
    this.ledColours = new Array(this.nLeds).fill(null);
    this.ledHues = new Array(this.nLeds).fill(null);
    this.setColour(ledHue, 0);
    this.ledSpacing = 0.3;
    this.edgeSpacing = 0.5;
    this.bar =
    {
      w: 0,
      h: 0
    }
    
    this.horizontal = horizontal;

    this.calcBarSize();
    this.ledPos = new Array(this.nLeds).fill();
    this.calcLedPos();
    
    this.edgeRounding = 0.2;
    
  }
  
  setColour(ledHue, alphaLevel)
  {
    for(let i = 0; i < this.nLeds; i++)
      this.setLedColour(i, ledHue, alphaLevel);
  }
  
  setLedLevel(i, level)
  {
    if(i < this.nLeds)
    {
      let alpha = map(level, 0, 1, 0.2, 1);
      let ledColourStr = "hsla("+str(this.ledHues[i])+", 100%, 50%," + alpha +")";
      this.ledColours[i] = color(ledColourStr);
    }
  }
  
  setLedColour(i, ledHue, alphaLevel)
  {
    if(i < this.nLeds)
    {
      this.ledHues[i] = ledHue;
      this.setLedLevel(i, alphaLevel);
    }
  } 
  
  setEdgeSpacing(s)
  {
    this.edgeSpacing = s;
    this.calcBarSize();
  }
  
  setEdgeRounding(s)
  {
    this.edgeRounding = s;
  }
  
  setLedSpacing(s)
  {
    this.ledSpacing = s;
    this.calcLedPos();
    this.calcBarSize();
  }
  
  
  setBarValue(val, fine=true)
  {
    let v = map(val, 0, 1, 0, this.nLeds);
    let v_floor = floor(v);
    let v_decimal = v - v_floor;
    for(let i = 0; i < this.nLeds; i++)
    {
      if(i < v_floor)
      {
        this.setLedLevel(i, 1);
      }
      else
      {
        if(fine && i == v_floor)
          this.setLedLevel(i, v_decimal);
        else
          this.setLedLevel(i, 0);
      }
    }
    
    
  }
  
  calcLedPos()
  {
    
    for(let i = 0; i < this.nLeds; i++)
    {
      let index  = i;
      if(this.horizontal)
      {
        this.ledPos[index] = this.x;
      }
      else
      {
          index = this.nLeds -1 - i;
          this.ledPos[index] = this.y;
      }
      this.ledPos[index] =  this.ledPos[index] + i * (1 + this.ledSpacing) * this.led.w;
      this.ledPos[index] = this.ledPos[index] + 0.5 * this.led.w;
      this.ledPos[index] = this.ledPos[index] + 0.5 * this.ledSpacing * this.led.w;
      this.ledPos[index] = this.ledPos[index] - this.nLeds * 0.5 * (this.led.w + this.led.w * this.ledSpacing);          

    }

  }
  
  calcBarSize()
  {
    this.bar.w = this.nLeds * this.led.w + (this.nLeds - 1) * this.led.w * this.ledSpacing + 2 * this.led.w * this.edgeSpacing;
    this.bar.h = this.led.h + 2 * this.led.w * this.edgeSpacing;
    
    if(!this.horizontal)
    {
      let h = this.bar.h
      let w = this.bar.w
      this.bar.h = w
      this.bar.w = h
    }
  }
  
  position(x, y, display=true)
  {
    this.x = x;
    this.y = y;
    this.calcLedPos();
    if(display)
      this.draw();
  } 
  
  draw()
  {
    push();
    rectMode(CENTER);
    noStroke();
    // Switch outline
    fill(128);
    rect(this.x, this.y, this.bar.w, this.bar.h);

    // noStroke();
    for(let i = 0; i < this.nLeds; i++)
    {
      fill(this.ledColours[i]);
      if(this.horizontal)
      {
        rect(this.ledPos[i], this.y, this.led.w, this.led.h, this.led.w * this.edgeRounding);
      }
      else
      {
        rect(this.x, this.ledPos[i], this.led.h, this.led.w, this.led.w * this.edgeRounding)
      }
    }
    pop();
  }
}