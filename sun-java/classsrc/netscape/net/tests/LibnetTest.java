import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Font;
import java.applet.Applet;
import java.net.URL;
import java.net.URLConnection;
import netscape.net.URLStreamHandler;
import java.io.InputStream;
import java.awt.FontMetrics;

public class LibnetTest extends Applet {

    public void paint(Graphics g) {
	InputStream s = null;
	try {
	    g.drawString("going to open connection", 10, 10);
	    URL myURL = new URL("http://warp/engr/java/demos/urldemo.html");
	    URLConnection con = myURL.openConnection();
	    s = con.getInputStream();
	    g.drawString("Connection opened! content-type="+con.getContentType()
			 +" len="+con.getContentLength(), 10, 30);
	    char[] ch = new char[1];
	    int c;
	    FontMetrics fm = g.getFontMetrics();
	    int x = 10;
	    int y = 50;
	    int lineHeight = fm.getHeight();
	    while ((c = s.read()) != -1) {
		ch[0] = (char)c;
		g.drawChars(ch, 0, 1, x, y);
		if (ch[0] == 12) {	// newline
		    x = 10;
		    y += lineHeight;
		}
		else
		    x += fm.charWidth(ch[0]);
	    }
	}
	catch (Exception e) {
	    if (s != null) {
		try {
		    s.close();
		    s.read();
		}
		catch (Exception e2) {
		    g.drawString("Bogus exception: "+e2.toString(), 10, 70);
		}
	    } else {
		g.drawString("Connection failed: "+e.toString(), 10, 70);
	    }
	}
    }
}
