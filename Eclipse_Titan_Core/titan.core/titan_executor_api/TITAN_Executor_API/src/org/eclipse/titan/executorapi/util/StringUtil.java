/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Lovassy, Arpad
 *
 ******************************************************************************/
package org.eclipse.titan.executorapi.util;

import java.lang.reflect.Array;
import java.util.List;
import java.util.Map;

/**
 * String related utility functions, which are used for logging and in toString() functions.
 * <p>
 * Dependencies (these functions are needed by):
 *   appendObject(): Log.fi(), Log.fo(), ComponentStruct.toString(), HostStruct.toString()
 *   toObjectArray(): appendObject()
 *   isPrimitiveArray: appendObject()
 *   indent(): ComponentStruct.toString()
 *   replaceString(): indent(), Log.f()
 */
public class StringUtil {
	
	/**
	 * Replaces all the occurrences of a given string
	 * @param aSb [in/out] the string to modify 
	 * @param aFrom [in] the string to replace
	 * @param aTo [in] the replacement string
	 */
	public static void replaceString( final StringBuilder aSb, final String aFrom, final String aTo ) {
		int index = aSb.indexOf( aFrom );
		while (index != -1) {
	        aSb.replace( index, index + aFrom.length(), aTo );
	        index += aTo.length(); // Move to the end of the replacement
	        index = aSb.indexOf( aFrom, index );
	    }
	}
	
	/**
	 * Appends any Object to the given StringBuilder.
	 * <p>
	 * It works with the following data types:
	 * <ul>
	 *   <li> null
	 *   <li> String
	 *   <li> array of Object
	 *   <li> array of any primitive type, like int[], boolean[], ...
	 *   <li> list of Object
	 * </ul>
	 * @param aSb [in/out] the StringBuilder to extend
	 * @param aObject [in] object to add
	 */
	public static void appendObject( final StringBuilder aSb, final Object aObject ) {
		final Object o = aObject;
		if ( o == null ) {
			aSb.append("null");
		}
		else if ( o instanceof String ) {
			aSb.append("\"");
			aSb.append(o.toString());
			aSb.append("\"");
		}
		else if ( o instanceof Object[] ) {
			final Object[] oArray = (Object[])o;
			aSb.append("[");
			if ( oArray.length > 0 ) {
				aSb.append(" ");
				for ( int i = 0; i < oArray.length; i++ ) {
					if ( i > 0 ) {
						aSb.append(", ");
					}
					appendObject(aSb, oArray[i]);
				}
				aSb.append(" ");
			}
			aSb.append("]");
		}
		else if ( isPrimitiveArray( o ) ) {
			appendObject( aSb, toObjectArray( o ) );
		}
		else if ( o instanceof List ) {
			final List<?> oList = (List<?>)o;
			aSb.append("[");
			if ( oList.size() > 0 ) {
				aSb.append(" ");
				for ( int i = 0; i < oList.size(); i++ ) {
					if ( i > 0 ) {
						aSb.append(", ");
					}
					appendObject(aSb, oList.get(i));
				}
				aSb.append(" ");
			}
			aSb.append("]");
		}
		else if ( o instanceof Map ) {
			final Map<?,?> oMap = (Map<?,?>)o;
			aSb.append("[");
			if ( oMap.size() > 0 ) {
				aSb.append(" ");
				int i = 0;
				for (Object key : oMap.keySet()) {
					if ( i++ > 0 ) {
						aSb.append(", ");
					}
					appendObject(aSb, key);
					aSb.append(": ");
					appendObject(aSb, oMap.get( key ) );
				}
				aSb.append(" ");
			}
			aSb.append("]");
		}
		else {
			aSb.append(o.toString());
		}
	}

	/**
	 * Changes the indentation of the string, a requested string is appended after every EOFs in the string. 
	 * @param aSb [in/out] the string to modify
	 * @param aIndentString [in] the requested string to append
	 */
	public static void indent( final StringBuilder aSb, final String aIndentString ) {
		replaceString( aSb, "\n", "\n" + aIndentString );
	}
	
	/**
	 * @param aPrimitiveArrayCandidate the object to check
	 * @return true, if aPrimitiveArrayCandidate is a primitive array, like int[], boolean[], ...
	 *         false otherwise
	 */
	private static boolean isPrimitiveArray( final Object aPrimitiveArrayCandidate ) {
		return aPrimitiveArrayCandidate != null &&
		       aPrimitiveArrayCandidate.getClass().isArray() &&
		       aPrimitiveArrayCandidate.getClass().getComponentType().isPrimitive();
	}
	
	/**
	 * Converts primitive array to Object array.
	 * <p>
	 * Prerequisite: aPrimitiveArray must be primitive array or Object array
	 * @param aPrimitiveArray a primitive array, like int[], boolean[], ...
	 * @return result Object[] representation of aPrimitiveArray
	 * @see <a href="http://stackoverflow.com/questions/5606338/cast-primitive-type-array-into-object-array-in-java">stackoverflow.com</a>
	 * @see #isPrimitiveArray(Object)
	 */
	private static Object[] toObjectArray( final Object aPrimitiveArray ) {
	    if ( aPrimitiveArray instanceof Object[] ) {
	       return ( Object[] )aPrimitiveArray;
	    }
	    int arrlength = Array.getLength( aPrimitiveArray );
	    Object[] outputArray = new Object[ arrlength ];
	    for(int i = 0; i < arrlength; ++i) {
	       outputArray[i] = Array.get( aPrimitiveArray, i );
	    }
	    return outputArray;
	}

}
