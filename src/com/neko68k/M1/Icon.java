package com.neko68k.M1;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

import android.graphics.Bitmap;

class ICONDIR{
	public short reserved; 		// always 0
	public short type;			// 1 for ico, 2 for cur
	public short numImg;


	public ICONDIR(){};
	
					
}

class ICONDIRENTRY{
	public byte width;
	public byte height;
	public byte numColors;
	public byte reserved; 		// always 0
	public short colorPlanes;	// 0 or 1
	public short bpp;
	public int sizeInBytes;
	public int pixelOffset;

	public ICONDIRENTRY(){};

	
	
}

class BITMAPINFOHEADER{
	public int biSize;
	public long biWidth;
	public long biHeight;
	public short biPlanes;
	public short biBitCount;
	public int biCompression;
	public int biSizeImage;
	public int biXPelsPerMeter;
	public int biYPelsPerMeter;
	public int biClrUsed;
	public int biClrImportant;
}

public class Icon {
	ICONDIR id;
	ICONDIRENTRY ide;
	String fn;
	
	private static FileInputStream inputStream;
	private static DataInputStream dInputStream; 
	
	public Icon(String in) {
		fn = in;
		File file = new File(in);
		try{
            inputStream = new FileInputStream(file);
            dInputStream = new DataInputStream(inputStream);
            }
            catch (FileNotFoundException e)
            {
                    e.printStackTrace();
            }
		read();
	}

	private Bitmap Convert(){
		Bitmap bm=null;
		
		return(bm);
	}
	
	private void read(){
		try {
			id.reserved = dInputStream.readShort();
			id.type = dInputStream.readShort();
			id.numImg = dInputStream.readShort();
			
			ide.width = dInputStream.readByte();
			ide.height = dInputStream.readByte();
			ide.numColors = dInputStream.readByte();
			ide.reserved = dInputStream.readByte();
			ide.colorPlanes = dInputStream.readShort();
			ide.bpp = dInputStream.readShort();
			ide.sizeInBytes = dInputStream.readInt();
			ide.pixelOffset = dInputStream.readInt();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
}

























