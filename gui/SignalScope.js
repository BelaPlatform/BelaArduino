class SignalScope
{
  constructor(width, height)
  {
    this.w = width;
    this.h = height;
    
    this.x = 0;
    this.y = 0;
    
    this.edgeRounding = 0;
    this.drawAxis = false;
    this.buffer = new Array();
    
    
    this.screenAxis = 
    {
      start_x: this.x - 0.5 * this.w,
      end_x: this.x + 0.5 * this.w,
      start_y: this.y - 0.5 * this.h,
      end_y: this.y + 0.5 * this.h
    }
    
    this.freeze = false;
    
  }
  
  position(x, y, display=true)
  {
    this.x = x;
    this.y = y;
    this.calcScreenAxis();
    if(display)
      this.draw();
  }
  
  addSample(s)
  {
    if(!this.freeze)
    {
      this.buffer.push(s);
      if(this.buffer.length >= this.w)
        this.buffer.splice(0, 1);
    }
  }
  
  calcScreenAxis()
  {
    this.screenAxis.start_x = this.x - 0.5 * this.w;
    this.screenAxis.end_x = this.x + 0.5 * this.w;
    this.screenAxis.start_y = this.y - 0.5 * this.h;
    this.screenAxis.end_y = this.y + 0.5 * this.h;
  }
  
  processSample(s)
  {
    let sample = map (s, 0, 1, this.screenAxis.end_y, this.screenAxis.start_y);
    sample = constrain(sample, this.screenAxis.start_y, this.screenAxis.end_y);
    return sample;
  }
  
  doFreeze()
  {
    this.freeze = !this.freeze;
  }
  
  click()
  {
    if(mouseButton === LEFT)
    {
      if(mouseX > this.x - 0.5 * this.w && mouseX < this.x + 0.5 * this.w)
      {
        if(mouseY > this.y - 0.5 * this.h && mouseY < this.y + 0.5 * this.h)
        {
          this.doFreeze();
        }
      }
    }
  }
  
  draw()
  {
    push();
    rectMode(CENTER);
    noStroke();
    // Switch outline
    if(this.freeze)
      fill(64);
    else
      fill(0);
    rect(this.x, this.y, this.w, this.h, this.h * this.edgeRounding);

    stroke(255);
    strokeWeight(this.h / 100);

    if(this.drawAxis)
    {
      line(this.screenAxis.start_x, this.y, this.screenAxis.end_x, this.y);
      line(this.x, this.screenAxis.start_y, this.x, this.screenAxis.end_y);
    }
    
    noFill();
    beginShape();
    stroke(255)
    for (let i = 0; i < this.buffer.length; i++) {
      let y = this.processSample(this.buffer[i]);
      vertex(this.screenAxis.start_x+i, y);
    }
    endShape();
    
    pop();
  }
  
}
