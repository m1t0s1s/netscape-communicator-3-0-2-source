package sun.awt.macos;

import java.awt.Graphics;
import java.awt.image.ImageProducer;

class MacImage extends sun.awt.image.Image {

    public MacImage(int w, int h) {
		super(w, h);
    }

    public MacImage(ImageProducer producer) {
		super(producer);
    }

    public Graphics getGraphics() {
		return new MacGraphics(this);
    }
    
    protected sun.awt.image.ImageRepresentation getImageRep(int w, int h) {
		return super.getImageRep(w, h);
    }
}
