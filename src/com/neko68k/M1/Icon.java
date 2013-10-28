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


	public ICONDIR(){}
	
					
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

	public ICONDIRENTRY(){}

	
	
}

class BITMAPINFOHEADER{
	public int biSize;
	public long biWidth;
	public long biHeight;
	public short biPlanes;
	public short biBitCount;
	public int biCompression;
	public int biSizeImage;
	public long biXPelsPerMeter;
	public long biYPelsPerMeter;
	public int biClrUsed;
	public int biClrImportant;
	
	public BITMAPINFOHEADER(){}
}

public class Icon {
	ICONDIR id;
	ICONDIRENTRY ide;
	BITMAPINFOHEADER bmih;
	String fn;
	
	byte[] bitmapPixels;
	byte[] palette;
	byte[] andMask;
	int finalPixels;
	final int maskSize = 32*32/4;
	
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
			// read icon headers
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
			
			// read bitmap header including palette
			// image bits follow 32x32 
			// AND mask seems to follow color bits
			
			bmih.biSize = dInputStream.readInt();
			bmih.biWidth = dInputStream.readLong();
			bmih.biHeight = dInputStream.readLong();
			bmih.biPlanes = dInputStream.readShort();
			bmih.biBitCount = dInputStream.readShort();
			bmih.biCompression = dInputStream.readInt();
			bmih.biSizeImage = dInputStream.readInt();
			bmih.biXPelsPerMeter = dInputStream.readLong();
			bmih.biYPelsPerMeter = dInputStream.readLong();
			bmih.biClrUsed = dInputStream.readInt();
			bmih.biClrImportant = dInputStream.readInt();
			
			switch(bmih.biBitCount){
			case 32:
				bitmapPixels = new byte[32*32*4];
				dInputStream.read(bitmapPixels, 0, 32*32*4);
				break;
			case 16:
				bitmapPixels = new byte[32*32*2];
				dInputStream.read(bitmapPixels, 0, 32*32*2);
				break;
			case 8:
				bitmapPixels = new byte[32*32];
				palette = new byte[256*4];
				andMask = new byte[maskSize];
				dInputStream.read(palette, 0, 256*4);
				dInputStream.read(bitmapPixels, 0, 32*32);
				dInputStream.read(andMask, 0, maskSize);
				break;
			case 4:
				bitmapPixels = new byte[32*32/2];
				palette = new byte[16*4];
				andMask = new byte[maskSize];
				dInputStream.read(palette, 0, 16*2);
				dInputStream.read(bitmapPixels, 0, 32*32/2);
				dInputStream.read(andMask, 0, maskSize);
				break;
			case 2:
				bitmapPixels = new byte[32*32/4];
				palette = new byte[4*4];
				andMask = new byte[maskSize];
				dInputStream.read(palette, 0, 4*4);
				dInputStream.read(bitmapPixels, 0, 32*32/4);
				dInputStream.read(andMask, 0, maskSize);
				break;
			case 1:
				bitmapPixels = new byte[32*32/8];
				palette = new byte[2*4];
				andMask = new byte[maskSize];
				dInputStream.read(palette, 0, 2*4);
				dInputStream.read(bitmapPixels, 0, 32*32/8);
				dInputStream.read(andMask, 0, maskSize);
				break;
			default:
				break;
			}
			dInputStream.close();
			inputStream.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
}

























