class HSlider 
{
  constructor(width, height)
  {
    this.w = width;
    this.h = height;
    
    this.x = 0;
    this.y = 0;
    
    this.sOffset = 0.2
    
    this.slider =
    {
      w: this.h * (1 - this.sOffset),
      xmin:this.x,
      xmax: this.x,
      x: this.x,
      color: 0 //color(255, 0, 0)
    };
    this.slider.xmin = this.x - 0.5 * this.w + 0.5 * this.slider.w + 0.5 * this.h * this.sOffset; 
    this.slider.xmax = this.x + 0.5 * this.w - 0.5 * this.slider.w - 0.5 * this.h * this.sOffset;
  
    
    this.draggable = false;
    this.precision = 1;
    this.textString = 0.5;
    this.setValue(0.5);
    
    this.value = this.getValue();
    this.previousValue = this.value;
    
    this.edgeRounding = 0.2;
  }
  
  position(x, y, display=true)
  {
    this.x = x;
    this.y = y;
    this.calcSliderPosition();
    if(display)
      this.draw();
  } 
  
  makeDraggable(d)
  {
    if(d === true || d === false)
    {
      this.draggable = d;
    }
  }
  
  getValue(precision = -1)
  {
    let ret = map(this.slider.x, this.slider.xmin, this.slider.xmax, 0, 1);
    if(precision != -1)
    {
      let precMult = Math.pow(10, precision);
      ret = floor(precMult * ret)/precMult;
    }
    return ret;
  }
  
  getRawValue()
  {
    return this.value;
  }
  
  setValue(val)
  {
    this.previousValue = this.value;
    this.value = constrain(val, 0, 1);
    let x = map(this.value, 0, 1, this.slider.xmin, this.slider.xmax);
    this.moveSlider(x);
    this.textString = this.getValue(this.precision);
  }
  
  hasChanged()
  {
    return (this.value != this.previousValue); 
  }
  
  setPrecision(p)
  {
    this.precision = p;
  }
  
  calcSliderPosition()
  {
    this.slider.x = this.x;
    this.slider.xmin = this.x - 0.5 * this.w + 0.5 * this.slider.w + 0.5 * this.h * this.sOffset;
    this.slider.xmax = this.x + 0.5 * this.w - 0.5 * this.slider.w - 0.5 * this.h * this.sOffset;
    this.textString = this.getValue(this.precision);
  }
  
  moveSlider(x)
  {
    this.slider.x = constrain(x, this.slider.xmin, this.slider.xmax);
  }
  
  mouseOverSlider()
  {
    if(mouseX >= this.slider.x - 0.5 * this.slider.w && mouseX <= this.slider.x + 0.5 * this.slider.w && mouseY >= this.y - 0.5 * this.slider.w && mouseY <= this.y + 0.5 * this.slider.w)
      {
        return true;
      }
    return false;
  }
  
  click()
  {
    if(this.mouseOverSlider())
    {
      this.makeDraggable(true);
      this.slider.color = color(0, 0, 0);
    }
  }
  
  doubleClick()
  {
      if(mouseX >= this.x - 0.5 * this.w && mouseX <= this.x + 0.5 * this.w && mouseY >= this.y - 0.5 * this.h && mouseY <= this.y + 0.5 * this.h)
        {
              this.slider.color = color(255, 165, 0);
        }
  }
  
  release()
  {
      this.makeDraggable(false);
  }
  
  draw()
  {
    push();
    rectMode(CENTER);
    noStroke();
    // Switch outline
    fill(128);
    rect(this.x, this.y, this.w, this.h, this.h * this.edgeRounding);
    
    fill(this.slider.color);
    if(this.draggable)
    {
      fill(64);
      this.moveSlider(mouseX);
      this.textString = this.getValue(this.precision);
    }
    
    square(this.slider.x, this.y, this.slider.w, this.slider.w * this.edgeRounding);
    
    fill(0);
    stroke(0);
    textAlign(LEFT, CENTER);
    textSize(0.65 * this.slider.w);
    text(this.textString, this.x + this.w * 0.5 + this.slider.w * 0.2, this.y);
    pop();
  }
}