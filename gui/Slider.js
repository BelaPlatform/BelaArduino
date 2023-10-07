class Slider 
{
  constructor(width, height, horizontal = true)
  {
    this.horizontal = horizontal;

    this.w = width;
    this.h = height;
 

    this.x = 0;
    this.y = 0;
    
    this.sOffset = 0.2
    
    this.slider =
    {
      w: this.h * (1 - this.sOffset),
      min_p:this.x,
      max_p: this.x,
      p: this.x,
      color: 0 //color(255, 0, 0)
    };
    if(!this.horizontal)
      this.slider.w = this.w * (1 - this.sOffset);
    this.calcSliderPosition();
  
    
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
    let ret;
    if(this.horizontal)
      ret = map(this.slider.p, this.slider.min_p, this.slider.max_p, 0, 1);
    else
      ret = map(this.slider.p, this.slider.min_p, this.slider.max_p, 1, 0);

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
    let x = map(this.value, 0, 1, this.slider.min_p, this.slider.max_p);
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
    if(this.horizontal)
    {
      this.slider.p = this.x;
      this.slider.min_p = this.slider.p - 0.5 * this.w + 0.5 * this.slider.w + 0.5 * this.h * this.sOffset;
      this.slider.max_p = this.slider.p + 0.5 * this.w - 0.5 * this.slider.w - 0.5 * this.h * this.sOffset;
    }
    else
    {
      this.slider.p = this.y;
      this.slider.max_p = this.slider.p + 0.5 * this.h - 0.5 * this.slider.w - 0.5 * this.w * this.sOffset;
      this.slider.min_p = this.slider.p - 0.5 * this.h + 0.5 * this.slider.w + 0.5 * this.w * this.sOffset;
    }
   
    this.textString = this.getValue(this.precision);
  }
  
  moveSlider(x)
  {
    this.slider.p = constrain(x, this.slider.min_p, this.slider.max_p);
  }
  
  mouseOverSlider()
  {
 
    if(this.horizontal)
    {
      if(mouseX >= this.slider.p - 0.5 * this.slider.w && mouseX <= this.slider.p + 0.5 * this.slider.w && mouseY >= this.y - 0.5 * this.slider.w && mouseY <= this.y + 0.5 * this.slider.w)
      {
        return true;
      }
    }
    else
    {
      if(mouseX >= this.x - 0.5 * this.slider.w && mouseX <= this.x + 0.5 * this.slider.w && mouseY >= this.slider.p - 0.5 * this.slider.w && mouseY <= this.slider.p + 0.5 * this.slider.w)
      {
        return true;
      }
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
    
    if(this.horizontal)
      rect(this.x, this.y, this.w, this.h, this.h * this.edgeRounding);
    else
      rect(this.x, this.y, this.w, this.h, this.w * this.edgeRounding);
    
    fill(this.slider.color);
    if(this.draggable)
    {
      fill(64);
      if(this.horizontal)
        this.moveSlider(mouseX);
      else
        this.moveSlider(mouseY);
      this.textString = this.getValue(this.precision);
    }
    
    if(this.horizontal)
      rect(this.slider.p, this.y, this.slider.w, this.slider.w, this.slider.w * this.edgeRounding);
    else
      rect(this.x, this.slider.p, this.slider.w, this.slider.w, this.slider.w * this.edgeRounding);

    
    fill(0);
    stroke(0);
    if(this.horizontal)
      textAlign(LEFT, CENTER);
    else
      textAlign(CENTER, CENTER);

    textSize(0.65 * this.slider.w);
    if(this.horizontal)
      text(this.textString, this.x + this.w * 0.5 + this.slider.w * 0.2, this.y);
    else
      text(this.textString, this.x, this.y + this.h * 0.5 + this.slider.w * 0.4);

    pop();
  }
}