class ToggleSwitch
{
  constructor(width, offTxt, onTxt, colors)
  {
    this.w = width;
    this.x = 0;
    this.y = 0;
     
    this.swOffset = 0.2;

    this.button = 
    {
      w: this.w,
      offset: this.swOffset  
    }
    this.button.w = this.w * (1 - this.swOffset);
    this.button.offset = this.button.w  * (0.5 + this.swOffset * 0.5);
    
    this.state = 1;
    this.previousState = 1;
    
    this.edgeRounding = 0.2;
    this.offTxt = offTxt ? offTxt : 'O';
    this.onTxt = onTxt ? onTxt : 'I';
    
    if(!colors)
      colors = {
        outline: 128,
        active: 0,
        text: 191,
      }
    this.colors = colors;
    this.visible = true;
  }
  
  position(x, y, display=true)
  {
    this.x = x;
    this.y = y;
    if(display)
      this.draw();
  } 
  
  setState(s)
  {
    this.state = (s == 1) ? 1 : 0;
  }
  
  getState()
  {
    return this.state;
  }
  
  hasChanged()
  {
    return (this.state != this.previousState);
  }
  
  draw()
  {
    this.previousState = this.state;
    if(!this.visible)
      return;
    
    push();
    rectMode(CENTER);
    noStroke();
    // Switch outline
    fill(this.colors.outline);
    rect(this.x, this.y, this.w, 2 * this.w, this.w*this.edgeRounding);
    
    // Switch toggle
    fill(this.colors.active);
    let squareY;
    let textY;
    let textString = "";
    if(this.state == 1)
    {
      squareY = this.y - this.button.offset;
      textY = this.y + this.button.offset;
      textString = this.onTxt;
    } else {
      squareY = this.y + this.button.offset;
      textY = this.y - this.button.offset;
      textString = this.offTxt;
    }
    rect(this.x, squareY, this.button.w, this.button.w, this.button.w*this.edgeRounding);
    fill(this.colors.text);
    stroke(this.colors.text);
    textAlign(CENTER, CENTER);
    textSize(0.65 * this.button.w);
    text(textString, this.x, textY, this.button.w, this.button.w);

    pop();

  }
  
  click()
  {
    if(mouseButton === LEFT)
    {
      if(mouseX > this.x - 0.5 * this.w && mouseX < this.x + 0.5 * this.w)
      {
        if(mouseY > this.y - this.w && mouseY < this.y + 0.5 * this.w)
        {
                  this.setState(!this.getState());
                  return this.getState();
        }
      }
    }
    else if(mouseButton === RIGHT)
    {
    }
  }
  show(visible)
  {
    this.visible = visible;
  }
}
