
import java.awt.*;

class FrameTest extends Frame {
    FrameTest() {
	reshape(10, 10, 100, 100);
	show();
	reshape(100, 30, 200, 120);
    }
    public void paint(Graphics g) {
	for (int x = 0 ; x < 500 ; x += 10) {
	    int len = 2;
	    if ((x % 50) == 0) {
		len += 5;
	    }
	    if ((x % 100) == 0) {
		len += 5;
	    }
	    g.drawLine(x, 0, x, len);
	    g.drawLine(0, x, len, x);
	}
    }
    public static void main(String argv[]) {
	new FrameTest();
    }
}
