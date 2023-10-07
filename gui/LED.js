class LED
{
  constructor(ledHue, diameter)
  {
    this.setColour(ledHue);
    this.diameter = diameter;
    this.x = 0;
    this.y = 0;
    this.state = 0;
    this.previousState = this.state;
    this.clickable = false;
  }
  
  setColour(ledHue)
  {
    this.ledHue = ledHue;
    let ledOffColourStr = "hsla("+str(ledHue)+", 100%, 50%, 0.2)";
    this.ledOffColour = color(ledOffColourStr);
    let ledOnColourStr = "hsl("+str(ledHue)+",  100%, 50%)";
    this.ledOnColour = color(ledOnColourStr);
    let ledReflectionColourStr = "hsl("+str(ledHue)+", 100%, 85%)";
    this.ledReflectionColour = color(ledReflectionColourStr);

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
    // this.previousState = this.state;
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
  
  makeClickable(c)
  {
    this.clickable = (c == true) ? true : false;
    if(c == false)
        this.setState(0);
  }
  
  draw()
  {
    this.previousState = this.state;

    push();
    // LED Off
    fill(this.ledOffColour);
    noStroke();
    circle(this.x, this.y, this.diameter); 
    // LED On
    if(this.state == 1)
    {
      fill(this.ledOnColour);
      noStroke();
      circle(this.x, this.y, 0.85 * this.diameter); 
      // LED Reflection
      noFill();
      strokeWeight(this.diameter/10);
      stroke(this.ledReflectionColour);
      arc(this.x, this.y, 0.5 * this.diameter, 0.5 * this.diameter, 0 + 0.5 * QUARTER_PI , HALF_PI - 0.5 * QUARTER_PI);
    }
    pop();
  }
  
  click()
  {
    if(this.clickable)
    {
      if(mouseButton === LEFT)
      {
        let d = dist(mouseX, mouseY, this.x, this.y);
        if(d < 0.5 * this.diameter)
        {
          this.setState(!this.getState());
          return this.getState();
        }
      }
      else if(mouseButton === RIGHT)
      {
      }
    }
  }
}