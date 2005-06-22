
using System;

namespace jvario
{
	/// <summary>
	/// Summary description for SerialParser.
	/// </summary>
	public class SerialParser
	{
		public SerialParser()
		{
 
		}

		private string inputBuffer; 

        public void DataReceived(string Data)
        {
			if(Data == null) 
			{
			}
			else
			{
				// search for end character
				inputBuffer += Data;

				if (inputBuffer.IndexOf('\n', 0)>=0) 
				{
					ParseData();
				}
			
			}
        }

		private void ParseData() 
		{
			// eat garbage until start text
			int kstart = inputBuffer.IndexOf('$', 0);
			if (kstart<0)  
			{
				return;
			}
			int klen = kstart;
			inputBuffer = inputBuffer.Substring(klen, inputBuffer.Length-klen);

			// determine packet type
			// PBB50
			if (inputBuffer.Substring(0, 6)=="$PJV01")
			{
				skipTo("$PJV01");
				ParsePJV01();
				return;
			}

			if (inputBuffer.Substring(0, 6)=="$PBB50") 
			{
				skipTo("$PBB50");
				ParsePBB50();
				return;
			}	
			if (inputBuffer.Substring(0, 6)=="$PBJVA") 
			{
				skipTo("$PBJVA");
				ParsePBJVA();
				return;
			}
			if (inputBuffer.Substring(0, 6)=="$PBJVH") 
			{
				skipTo("$PBJVH");
				ParsePBJVH();
				return;
			}
			if (inputBuffer.Substring(0, 6)=="$GPGGA") 
			{
				skipTo("$GPGGA");
				ParseGPGGA();
				return;
			}
			if (inputBuffer.Substring(0, 6)=="$GPRMC") 
			{
				skipTo("$GPRMC");
				ParseGPRMC();
				return;
			}
			ParseEnd();
			//			klen--;
		}

		private bool skipTo(string intext) 
		{
			int kstart;
			kstart = inputBuffer.IndexOf(intext,0);
			if (kstart>=0)
			{
				int klen = kstart+intext.Length;
				inputBuffer = inputBuffer.Substring(klen, inputBuffer.Length-klen);
				return true;
			} 
			else return false;
		}

		
		private void ParsePJV01() 
		{
			double vval;
			if (!skipTo(",")) return; // skip start 

			try 
			{
				int kstart;
	
				kstart = inputBuffer.IndexOf(',',0);
				vval = System.Double.Parse(inputBuffer.Substring(0, kstart));
				lock(displayForm.sync) 
				{
					displayForm.getAirData().Vias = vval;
				}

				if (!skipTo(",")) return; // go to next field

				kstart = inputBuffer.IndexOf(',',0);
				vval = System.Double.Parse(inputBuffer.Substring(0, kstart));
				lock(displayForm.sync) 
				{
					displayForm.getAirData().wnet = vval;
				}

				if (!skipTo(",")) return; // go to next field 

				kstart = inputBuffer.IndexOf(',',0);
				vval = System.Double.Parse(inputBuffer.Substring(0, kstart));
				lock(displayForm.sync) 
				{
					displayForm.getAirData().updateStaticPressure(vval*10);
				}
			}
			catch (FormatException) 
			{
			}
			ParseEnd();
			lock(displayForm.sync) 
			{
				displayForm.updateAirData(true);
				displayForm.tick();
	//			displayForm.DoPaint();
			}
		}

		private void ParsePBB50() 
		{
			double vval;
			if (!skipTo(",")) return; // skip start 

			try 
			{
				int kstart;
	
				kstart = inputBuffer.IndexOf(',',0);
				vval = System.Double.Parse(inputBuffer.Substring(0, kstart));
				lock(displayForm.sync) 
				{
					displayForm.getAirData().Vtas = vval;
				}

				if (!skipTo(",")) return; // go to next field

				kstart = inputBuffer.IndexOf(',',0);
				vval = System.Double.Parse(inputBuffer.Substring(0, kstart));
				lock(displayForm.sync) 
				{
					displayForm.getAirData().wnet = vval;
				}

				if (!skipTo(",")) return; // go to next field (skip mcready setting)
				if (!skipTo(",")) return; // go to next field

				kstart = inputBuffer.IndexOf(',',0);
				vval = System.Double.Parse(inputBuffer.Substring(0, kstart));
				lock(displayForm.sync) 
				{
					displayForm.getAirData().Vias = Math.Sqrt(vval);
				}

			}
			catch (FormatException) 
			{
			}
			ParseEnd();
			lock(displayForm.sync) 
			{
				displayForm.updateAirData(true);
	//			displayForm.DoPaint();
			}
		}

		private void ParseGPGGA() 
		{


			if (!skipTo(",")) return; // start comma
			if (!skipTo(",")) return; // skip time

			if (!skipTo(",")) return; // skip lat
			if (!skipTo(",")) return; // skip lat hemisphere
			if (!skipTo(",")) return; // skip long
			if (!skipTo(",")) return; // skip long hemisphere
			if (!skipTo(",")) return; // skip sat info
			if (!skipTo(",")) return; // skip sat info
			if (!skipTo(",")) return; // skip epe

			try 
			{
				// altitude
				int kstart = inputBuffer.IndexOf(',',0);
				lock(displayForm.sync) 
				{
					displayForm.getAirData().updateGpsAltitude(
						System.Double.Parse(inputBuffer.Substring(0, kstart)));					
			//		displayForm.tick();
			//		displayForm.updateGPS();
			//		displayForm.DoPaint();
				}
			}
			catch (FormatException) 
			{
			}
			ParseEnd();	
		}

		private void ParseGPRMC() 
		{

			if (!skipTo(",")) return; // start comma
			if (!skipTo(",")) return; // skip time

			if (!skipTo(",")) return; // skip validity flag

			if (!skipTo(",")) return; // skip lat
			if (!skipTo(",")) return; // skip lat hemisphere
			if (!skipTo(",")) return; // skip long
			if (!skipTo(",")) return; // skip long hemisphere
			try 
			{
				// speed over ground
				int kstart = inputBuffer.IndexOf(',',0);
				lock(displayForm.sync) 
				{
					displayForm.getAirData().Vgnd = System.Double.Parse(inputBuffer.Substring(0, kstart));	
				}

				if (!skipTo(",")) return; // skip to next
	
				// course made good
				kstart = inputBuffer.IndexOf(',',0);
				lock(displayForm.sync) 
				{
					displayForm.getAirData().hdg_good = System.Double.Parse(inputBuffer.Substring(0, kstart));
				}

				// debugging only
				lock(displayForm.sync) 
				{
					displayForm.tick();
					displayForm.updateAirData(false);
					displayForm.updateGPS();
			//		displayForm.DoPaint();
				}
			}
			catch (FormatException) 
			{
			}
			ParseEnd();	
			
		}
	
		private void ParsePBJVA() 
		{
			double xval, zval;
			int kstart;
			int val;
			inputBuffer = inputBuffer.Replace(' ','0');
			try 
			{
				if (!skipTo(",")) return;
				kstart = inputBuffer.IndexOf(',',0);
				val = System.Int16.Parse(inputBuffer.Substring(0, kstart));
				xval = val/100.0;
	
				if (!skipTo(",")) return;
				kstart = inputBuffer.IndexOf('*',0);
				val = System.Int16.Parse(inputBuffer.Substring(0, kstart));
				zval = val/100.0;

				lock(displayForm.sync) 
				{
					displayForm.my_logic.my_accel.ReadData(xval, zval);
				}
				
			}
			catch (FormatException) 
			{
			}
			
			ParseEnd();
		}

		private void ParsePBJVH() 
		{
			int kstart;
			inputBuffer = inputBuffer.Replace(' ','0');
			try 
			{
				int oat;
				int rh;
				if (!skipTo(",")) return;
				kstart = inputBuffer.IndexOf(',',0);
				rh = System.Int16.Parse(inputBuffer.Substring(0, kstart));
	
				if (!skipTo(",")) return;
				kstart = inputBuffer.IndexOf('*',0);
				oat = System.Int16.Parse(inputBuffer.Substring(0, kstart));

				lock(displayForm.sync) 
				{
					displayForm.getAirData().OAT = oat/10.0-273.0;
					displayForm.getAirData().RH = rh/10.0;
					// displayForm.statusMsg("humid");
				}
			}
			catch (FormatException) 
			{
			}
			
			ParseEnd();
		}

		private void ParseEnd()
		{
			int kstart = inputBuffer.IndexOf('\n',0);
			if (kstart>=0) 
			{
				inputBuffer = inputBuffer.Substring(kstart+1,inputBuffer.Length-kstart-1);
			}
		}


		private JVario displayForm;

		public void setDisplay(JVario thedisplayForm)
		{
			displayForm = thedisplayForm;
		}

	}
}
