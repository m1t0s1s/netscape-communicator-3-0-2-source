/*
 * @(#)
 *
 * CAR file reader/writer code
 *
 * @author Kipp E.B. Hickman
 */
package netscape.tools.car;

class CarCheckException extends Exception {
    CarCheckException() {
	super();
    }
    CarCheckException(String what) {
	super(what);
    }
}
