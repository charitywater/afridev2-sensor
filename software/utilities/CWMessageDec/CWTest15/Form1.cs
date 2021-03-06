﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace cwtest
{
    public partial class Form1 : Form
    {
        public Form1()
        {

            InitializeComponent();
            if (textBoxFA_Message.Text.Length >=17)
                textBoxGMT_Time.Text = Convert.ToString(get_FA_time(textBoxFA_Message.Text.Substring(6, 12)));
            textBoxTZ_Hours.Text = "0";
            textBoxTZ_Minutes.Text = "0";
#if false
            checkBoxNewGPS.Checked = true;
            checkBoxExistingGPS.Checked = false;
            textBoxSatellite.Text = "4";
            textBoxHDOP.Text = "30";
            textBoxMeasTime.Text = "60";
#endif
            textBoxUnkLimit.Text = "90";
            textBoxWaterLimit.Text = "900";
            listViewDWL_LPH.View = View.Details;
            listViewDWL_LPH.GridLines = true;
            listViewDWL_LPH.FullRowSelect = true;
            listViewDWL_LPH.Columns.Add("Time", 60);
            listViewDWL_LPH.Columns.Add("Liters", 60);
        }


        private DateTime get_FA_time(string timestr)
        {
            DateTime myDate;



            if (timestr.Length >= 12)
            {
                string second_s = "0x" + timestr.Substring(0, 2);
                int second = Convert.ToInt32(second_s, 16);
                if (second >= 0 && second < 60)
                {
                    second_s = Convert.ToString(second);
                    if (second_s.Length == 1)
                        second_s = "0" + second_s;
                }
                else
                    second_s = "00";

                string minute_s = "0x" + timestr.Substring(2, 2);
                int minute = Convert.ToInt32(minute_s, 16);
                if (minute >= 0 && minute < 60)
                {
                    minute_s = Convert.ToString(minute);
                    if (minute_s.Length == 1)
                        minute_s = "0" + minute_s;
                }
                else
                    minute_s = "00";

                string hour_s = "0x" + timestr.Substring(4, 2);
                int hour = Convert.ToInt32(hour_s, 16);
                if (hour >= 0 && hour < 24)
                {
                    hour_s = Convert.ToString(hour);
                    if (hour_s.Length == 1 && hour < 24 && hour >= 0)
                        hour_s = "0" + hour_s;
                }
                else
                    hour_s = "00";

                string day_s = "0x" + timestr.Substring(6, 2);
                int day = Convert.ToInt32(day_s, 16);
                if (day > 0 && day <= 31)
                {
                    day_s = Convert.ToString(day);
                    if (day_s.Length == 1)
                        day_s = "0" + day_s;
                }
                else
                    day_s = "01";

                string month_s = "0x" + timestr.Substring(8, 2);
                int month = Convert.ToInt32(month_s, 16);
                if (month > 0 && month <= 12)
                {
                    month_s = Convert.ToString(month);
                    if (month_s.Length == 1)
                        month_s = "0" + month_s;
                }
                else
                    month_s = "01";

                string year_s = "0x" + timestr.Substring(10, 2);
                int year = Convert.ToInt32(year_s, 16);
                if (year > 0 && year < 30)
                    year_s = Convert.ToString(year + 2000);
                else
                    year_s = "2018";

                string date_s = year_s + "-" + month_s + "-" + day_s + " " + hour_s + ":" + minute_s + ":" + second_s;

                myDate = DateTime.ParseExact(date_s, "yyyy-MM-dd HH:mm:ss",
                                       System.Globalization.CultureInfo.InvariantCulture);

                textBoxCSVmonth.Text = month_s + '/' + day_s + '/' + year_s;
                textBoxCSVhour.Text = hour_s + ":" + minute_s + ":" + second_s;
            }
            else
                myDate = DateTime.Now;

            return myDate;
        }

        private string space_bytes(string instr)
        {
            int i;
            string answer = "";

            for (i = 0; i < instr.Length; i++)
            {
                if (answer.Length > 0)
                    answer = answer + " ";

                if (i + 1 < instr.Length)
                {
                    answer = answer + instr.Substring(i, 2);
                    i++;
                }
                else
                    answer = answer + instr.Substring(i, 1);
            }

            return (answer);
        }
        private string fw_copy_result(int code)
        {
            string answer = "?";
            switch (code)
            {
                case 0x00: answer = "Success"; break;
                case 0xFF: answer = "Err No Backup Image"; break;
                case 0xFE: answer = "Err Bad Backup CRC"; break;
                case 0xFD: answer = "Err Copy Failed"; break;
                case 0xFC: answer = "Err Bad Main CRC"; break;
                default: answer = "Unknown"; break;
            }
            return (answer);
        }

        private string reboot_reason(int code)
        {
            string answer = "";

            // derived from the IFG1 register on the MSP430
            if ((code & 0x01)==0x01)
                answer = answer + "WDT Overflow,";
            if ((code & 0x02)==0x02)
                answer = answer + "Oscillator Fault,";
            if ((code & 0x04)==0x04)
                answer = answer + "Power On Reset,";
            if ((code & 0x08)==0x08)
                answer = answer + "External Reset,";
            if ((code & 0x10)==0x10)
                answer = answer + "RST/NMI,";

            return (answer);
        }

        private string message_id(int code)
        {
            string answer = "?";
            switch (code)
            {
                case 0x00: answer = "Final Assembly"; break;
                case 0x03: answer = "OTA Reply"; break;
                case 0x05: answer = "Monthly Check-In"; break;
                case 0x06: answer = "SOS (Boot Message)"; break;
                case 0x07: answer = "Activated"; break;
                case 0x08: answer = "GPS Location"; break;
                case 0x21: answer = "Daily Water Log"; break;
                case 0x22: answer = "Sensor Data"; break;
                case 0x23: answer = "SOS (Boot Message)"; break;
            }
            return (answer);
        }

        private string reply_message_id(int code)
        {
            string answer = "?";
            switch (code)
            {
                case 0x01: answer = "GMT Clock Update"; break;
                case 0x02: answer = "Storage Clock Alignment"; break;
                case 0x03: answer = "Reset Data"; break;
                case 0x04: answer = "Reset Red Flag"; break;
                case 0x05: answer = "Activate Device"; break;
                case 0x06: answer = "Deactivate Device"; break;
                case 0x07: answer = "Set Transmission Rate"; break;
                case 0x08: answer = "Reset Device"; break;
                case 0x0d: answer = "GPS Request"; break;
                case 0x0e: answer = "GPS Measurement Params"; break;
                case 0x0f: answer = "Sensor Request"; break;
                case 0x10: answer = "Firmware Update"; break;
                default: answer = "Unknown Reply"; break;
            }
            return (answer);
        }

        private string fw_error_code(int code)
        {
            string answer = "?";
            switch (code)
            {
                case 0: answer = "None"; break;
                case -1: answer = "Modem Error"; break;
                case -2: answer = "Section Header Error"; break;
                case -3: answer = "Parameter Error"; break;
                case -4: answer = "CRC Error"; break;
                case -5: answer = "Timeout Error"; break;
                default: answer = "Unknown Error"; break;
            }
            return (answer);
        }



        private string req_type(int code)
        {
            string answer = "?";
            switch (code)
            {
                case 0x00: answer = "Request Sensor Data"; break;
                case 0x01: answer = "Overwrite Factory Data"; break;
                case 0x02: answer = "Reset Water Detection"; break;
                case 0x03: answer = "Set Unknown Limit"; break;
                case 0x04: answer = "Report Water Flow Data Now"; break;
                case 0x05: answer = "Set Downspout Rate"; break;
                case 0x06: answer = "Set Water Limit"; break;
                case 0x07: answer = "Set Wake Time"; break;
                case 0x08: answer = "NOP Response"; break;
                case 0x09: answer = "Margin Growth Data"; break;
                default: answer = "Unknown Reqtype"; break;
            }
            return (answer);
        }

        public bool OnlyHexInString(string test)
        {
            // For C-style hex notation (0xFF) you can use @"\A\b(0[xX])?[0-9a-fA-F]+\b\Z"
            return System.Text.RegularExpressions.Regex.IsMatch(test, @"\A\b[0-9a-fA-F]+\b\Z");
        }

        public bool OnlyFFInString(string test)
        {
            // For C-style hex notation (0xFF) you can use @"\A\b(0[xX])?[0-9a-fA-F]+\b\Z"
            return System.Text.RegularExpressions.Regex.IsMatch(test, @"\A\b[fF]+\b\Z");
        }
        public string format_latitude(string raw_data)
        {
            string lats;
            string degrees;
            string minutes;
            string direction;

            int lat = int.Parse(raw_data, System.Globalization.NumberStyles.HexNumber);
            
            // latitude goes from -90 to 90 degrees and 0 to 60 seconds
            // we report seconds to 4 decimal points
            // negative latitude is South and positive is North
            // this function generates 90 59.9999N
            if (lat < 0)
            {
                direction = "S";
                lat = -lat;
            }
            else
                direction = "N";
            lats = Convert.ToString(lat);
            if (lats.Length > 6)
            {
                degrees = lats.Substring(0, lats.Length - 6);
                lats = lats.Substring(lats.Length - 6, 6);
            }
            else
                degrees = "0";
            if (lats.Length > 4)
            {
                minutes = lats.Substring(0, lats.Length - 4);
                lats = lats.Substring(lats.Length - 4, 4);
            }
            else
                minutes = "0";
            if (lats.Length > 0)
            {
                string pad0 = "0000";
                // pad with 0's the fractional part that isnt there (0.0123)
                if (lats.Length < 4)
                    minutes = minutes + "." + pad0.Substring(0, 4 - lats.Length) + lats;
                else
                    minutes = minutes + "." + lats;
            }
            lats = degrees + " " + minutes + direction;

            return (lats);
        }

        public string format_longitude(string raw_data)
        {
            string lons;
            string degrees;
            string minutes;
            string direction;

            int lon = int.Parse(raw_data, System.Globalization.NumberStyles.HexNumber);

            // latitude goes from -180 to 180 degrees and 0 to 60 seconds
            // we report seconds to 4 decimal points
            // negative latitude is West and positive is East
            // this function generates 180 59.9999W
            if (lon < 0)
            {
                direction = "W";
                lon = -lon;
            }
            else
                direction = "E";
            lons = Convert.ToString(lon);
            if (lons.Length > 6)
            {
                degrees = lons.Substring(0, lons.Length - 6);
                lons = lons.Substring(lons.Length - 6, 6);
            }
            else 
                degrees = "0";
            if (lons.Length > 4)
            {
                minutes = lons.Substring(0, lons.Length - 4);
                lons = lons.Substring(lons.Length - 4, 4);
            }
            else
                minutes = "0";
            if (lons.Length > 0)
            {
                string pad0 = "0000";
                // pad with 0's the fractional part that isnt there (0.0123)
                if (lons.Length < 4)
                    minutes = minutes + "." + pad0.Substring(0, 4 - lons.Length) + lons;
                else
                    minutes = minutes + "." + lons;
            }

            lons = degrees + " " + minutes + direction;

            return (lons);
        }

        void parse_timestamp_message()
        {
            if (textBoxFA_Message.Text.Length >= 58)
            {
                string systimes = textBoxFA_Message.Text.Substring(38, 2) + textBoxFA_Message.Text.Substring(36, 2) + textBoxFA_Message.Text.Substring(34, 2) + textBoxFA_Message.Text.Substring(32, 2);
                int systime = int.Parse(systimes, System.Globalization.NumberStyles.HexNumber);
                int sys_day = systime / 86400;
                systime -= sys_day * 86400;
                int sys_hour = systime / 3600;
                systime -= sys_hour * 3600;
                int sys_min = systime / 60;
                systime -= sys_min * 60;
                int sys_sec = systime;

                string sys_sec_s;
                if (sys_sec >= 0 && sys_sec < 60)
                {
                    sys_sec_s = Convert.ToString(sys_sec);
                    if (sys_sec_s.Length == 1)
                        sys_sec_s = "0" + sys_sec_s;
                }
                else
                    sys_sec_s = "00";

                string sys_min_s;
                if (sys_min >= 0 && sys_min < 60)
                {
                    sys_min_s = Convert.ToString(sys_min);
                    if (sys_min_s.Length == 1)
                        sys_min_s = "0" + sys_min_s;
                }
                else
                    sys_min_s = "00";

                string sys_hour_s;
                if (sys_hour >= 0 && sys_hour < 24)
                {
                    sys_hour_s = Convert.ToString(sys_hour);
                    if (sys_hour_s.Length == 1)
                        sys_hour_s = "0" + sys_hour_s;
                }
                else
                    sys_hour_s = "00";

                string sys_day_s;
                sys_day_s = Convert.ToString(sys_day);

                textBoxTimeSysTime.Text = sys_day_s + ":" + sys_hour_s + ":" + sys_min_s + ":" + sys_sec_s;
                string secs = textBoxFA_Message.Text.Substring(40, 2);
                string mins = textBoxFA_Message.Text.Substring(42, 2);
                string hours = textBoxFA_Message.Text.Substring(44, 2);
                string days = textBoxFA_Message.Text.Substring(48, 2);
                string mons = textBoxFA_Message.Text.Substring(46, 2);
                string years = textBoxFA_Message.Text.Substring(50, 2);
                int sec = int.Parse(secs, System.Globalization.NumberStyles.HexNumber);
                int min = int.Parse(mins, System.Globalization.NumberStyles.HexNumber);
                int hour = int.Parse(hours, System.Globalization.NumberStyles.HexNumber);
                int day = int.Parse(days, System.Globalization.NumberStyles.HexNumber);
                int mon = int.Parse(mons, System.Globalization.NumberStyles.HexNumber);
                int year = int.Parse(years, System.Globalization.NumberStyles.HexNumber);
                textBoxTimeRTC.Text = Convert.ToString(hour) + ":" + Convert.ToString(min) + ":" + Convert.ToString(sec) + " " + Convert.ToString(day) + "/" + Convert.ToString(mon) + "/" + Convert.ToString(year);
                string st_secs = textBoxFA_Message.Text.Substring(52, 2);
                string st_mins = textBoxFA_Message.Text.Substring(54, 2);
                string st_hours = textBoxFA_Message.Text.Substring(56, 2);
                int st_sec = int.Parse(st_secs, System.Globalization.NumberStyles.HexNumber);
                string st_sec_s;
                if (st_sec >= 0 && st_sec < 60)
                {
                    st_sec_s = Convert.ToString(st_sec);
                    if (st_sec_s.Length == 1)
                        st_sec_s = "0" + st_sec_s;
                }
                else
                    st_sec_s = "00";
                string st_min_s;
                int st_min = int.Parse(st_mins, System.Globalization.NumberStyles.HexNumber);
                if (st_min >= 0 && st_sec < 60)
                {
                    st_min_s = Convert.ToString(st_min);
                    if (st_min_s.Length == 1)
                        st_min_s = "0" + st_min_s;
                }
                else
                    st_min_s = "00";
                string st_hour_s;
                int st_hour = int.Parse(st_hours, System.Globalization.NumberStyles.HexNumber);
                if (st_hour >= 0 && st_hour < 24)
                {
                    st_hour_s = Convert.ToString(st_hour);
                    if (st_hour_s.Length == 1)
                        st_hour_s = "0" + st_hour_s;
                }
                else
                    st_hour_s = "00";
                textBoxTimeStorage.Text = st_hour_s + ":" + st_min_s + ":" + st_sec_s;

                textBoxCSVstring.Text = textBoxCSVwell.Text + "," + textBoxCSVmonth.Text + " " + textBoxCSVhour.Text + "," + textBoxTimeRTC.Text + "," + textBoxTimeSysTime.Text + "," + textBoxTimeStorage.Text;
            }
            else
            {
                textBoxTimeSysTime.Text = "";
                textBoxTimeStorage.Text = "";
                textBoxTimeRTC.Text = "";
            }
        }

        void parse_ota_reply_message()
        {
            if (textBoxFA_Message.Text.Length >= 48)
            {
                string opcodes = textBoxFA_Message.Text.Substring(32, 2);
                int opcode = int.Parse(opcodes, System.Globalization.NumberStyles.HexNumber);
                textBoxOR_Opcode.Text = reply_message_id(opcode);
                string msgnums = textBoxFA_Message.Text.Substring(34, 4);
                textBoxOR_Msgnum.Text = msgnums;
                string statuss = textBoxFA_Message.Text.Substring(38, 2);
                int status = int.Parse(statuss, System.Globalization.NumberStyles.HexNumber);
                textBoxOR_Status.Text = Convert.ToString(status);
                switch(opcode)
                {
                    case 1:
                        string accepts = textBoxFA_Message.Text.Substring(40, 2);
                        int accept = int.Parse(accepts, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Accept.Text = Convert.ToString(accept);
                        string gmtsecs = textBoxFA_Message.Text.Substring(42, 2);
                        int gmtsec = int.Parse(gmtsecs, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Gmtsec.Text = Convert.ToString(gmtsec);
                        string gmtmins = textBoxFA_Message.Text.Substring(44, 2);
                        int gmtmin = int.Parse(gmtmins, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Gmtmin.Text = Convert.ToString(gmtmin);
                        string gmthours = textBoxFA_Message.Text.Substring(46, 2);
                        int gmthour = int.Parse(gmthours, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Gmthour.Text = Convert.ToString(gmthour);
                        string gmtdays = textBoxFA_Message.Text.Substring(48, 4);
                        int gmtday = int.Parse(gmtdays, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Gmtday.Text = Convert.ToString(gmtday);
                        break;
                    case 2:
                        string gmt2secs = textBoxFA_Message.Text.Substring(40, 2);
                        int gmt2sec = int.Parse(gmt2secs, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Gmtsec.Text = Convert.ToString(gmt2sec);
                        string gmt2mins = textBoxFA_Message.Text.Substring(42, 2);
                        int gmt2min = int.Parse(gmt2mins, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Gmtmin.Text = Convert.ToString(gmt2min);
                        string gmt2hours = textBoxFA_Message.Text.Substring(44, 2);
                        int gmt2hour = int.Parse(gmt2hours, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Gmthour.Text = Convert.ToString(gmt2hour);
                        string alignsecs = textBoxFA_Message.Text.Substring(48, 2);
                        int alignsec = int.Parse(alignsecs, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Alignsec.Text = Convert.ToString(alignsec);
                        string alignmins = textBoxFA_Message.Text.Substring(50, 2);
                        int alignmin = int.Parse(alignmins, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Alignmin.Text = Convert.ToString(alignmin);
                        string alignhours = textBoxFA_Message.Text.Substring(52, 2);
                        int alignhour = int.Parse(alignhours, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Alignhour.Text = Convert.ToString(alignhour);
                        break;
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 8:
                        ; // nothing to do
                        break;
                    case 7:
                        string tranrates = textBoxFA_Message.Text.Substring(40, 2);
                        int tranrate = int.Parse(tranrates, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Tranrate.Text = Convert.ToString(tranrate);
                        break;
                    case 13:
                        if (textBoxFA_Message.Text.Length >= 70)
                        {
                            string hours = textBoxFA_Message.Text.Substring(40, 2);
                            int hour = int.Parse(hours, System.Globalization.NumberStyles.HexNumber);
                            textBoxGPS_Hours.Text = Convert.ToString(hour);
                            string minutes = textBoxFA_Message.Text.Substring(42, 2);
                            int minute = int.Parse(minutes, System.Globalization.NumberStyles.HexNumber);
                            textBoxGPS_Minutes.Text = Convert.ToString(minute);

                            textBoxGPS_Latitude.Text = format_latitude(textBoxFA_Message.Text.Substring(44, 8));
                            textBoxGPS_Longitude.Text = format_longitude(textBoxFA_Message.Text.Substring(52, 8));

                            string fixqs = textBoxFA_Message.Text.Substring(60, 2);
                            int fixq = int.Parse(fixqs, System.Globalization.NumberStyles.HexNumber);
                            checkBoxGPS_FixQ.Checked = fixq != 0 ? true : false;
                            string sats = textBoxFA_Message.Text.Substring(62, 2);
                            int sat = int.Parse(sats, System.Globalization.NumberStyles.HexNumber);
                            textBoxGPS_Sat.Text = Convert.ToString(sat);
                            string hdops = textBoxFA_Message.Text.Substring(64, 2);
                            int hdop = int.Parse(hdops, System.Globalization.NumberStyles.HexNumber);
                            hdops = Convert.ToString(hdop);
                            if (hdops.Length == 2)
                                textBoxGPS_HDOP.Text = hdops.Substring(0, 1) + "." + hdops.Substring(1, 1) + " m";
                            else if (hdops.Length == 3)
                                textBoxGPS_HDOP.Text = hdops.Substring(0, 2) + "." + hdops.Substring(2, 1) + " m";
                            else
                                textBoxGPS_HDOP.Text = "0." + hdops.Substring(0, 1) + " m";
                            string mts = textBoxFA_Message.Text.Substring(66, 4);
                            int mt = int.Parse(mts, System.Globalization.NumberStyles.HexNumber);
                            textBoxGPS_Time.Text = Convert.ToString(mt) + " sec";
                        }
                        else
                        {
                            textBoxGPS_Hours.Text = "";
                            textBoxGPS_Minutes.Text = "";
                            textBoxGPS_Latitude.Text = "";
                            textBoxGPS_Longitude.Text = "";
                            checkBoxGPS_FixQ.Checked = false;
                            textBoxGPS_Sat.Text = "";
                            textBoxGPS_HDOP.Text = "";
                            textBoxGPS_Time.Text = "";
                        }
                        break;
                    case 14:
                        string sat2s = textBoxFA_Message.Text.Substring(40, 2);
                        int sat2 = int.Parse(sat2s, System.Globalization.NumberStyles.HexNumber);
                        textBoxGPS_Sat.Text = Convert.ToString(sat2);
                        string hdop2s = textBoxFA_Message.Text.Substring(42, 2);
                        int hdop2 = int.Parse(hdop2s, System.Globalization.NumberStyles.HexNumber);
                        hdop2s = Convert.ToString(hdop2);
                        if (hdop2s.Length == 2)
                            textBoxGPS_HDOP.Text = hdop2s.Substring(0, 1) + "." + hdop2s.Substring(1, 1) + " m";
                        else if (hdop2s.Length == 3)
                            textBoxGPS_HDOP.Text = hdop2s.Substring(0, 2) + "." + hdop2s.Substring(2, 1) + " m";
                        else
                            textBoxGPS_HDOP.Text = "0." + hdop2s.Substring(0, 1) + " m";
                        string mts2 = textBoxFA_Message.Text.Substring(44, 4);
                        int mt2 = int.Parse(mts2, System.Globalization.NumberStyles.HexNumber);
                        textBoxGPS_Time.Text = Convert.ToString(mt2) + " sec";
                        break;
                    case 15:
                        string reqtypes = textBoxFA_Message.Text.Substring(40, 2);
                        int reqtype = int.Parse(reqtypes, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Reqtype.Text = req_type(reqtype);
                        string reqdatas = textBoxFA_Message.Text.Substring(42, 4);
                        int reqdata = int.Parse(reqdatas, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Reqdata.Text = Convert.ToString(reqdata);
                        break;
                    case 16:
                        string errcodes = textBoxFA_Message.Text.Substring(40, 2);
                        int errcode = int.Parse(errcodes, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Errcode.Text = fw_error_code(errcode);
                        string msgcrcs = textBoxFA_Message.Text.Substring(42, 4);
                        int msgcrc = int.Parse(msgcrcs, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Msgcrc.Text = Convert.ToString(msgcrc);
                        string calccrcs = textBoxFA_Message.Text.Substring(46, 4);
                        int calccrc = int.Parse(calccrcs, System.Globalization.NumberStyles.HexNumber);
                        textBoxOR_Calccrc.Text = Convert.ToString(calccrc);
                        break;
                    default:
                        break;

                }
            }
            else
            {
                textBoxSOS_Reason.Text = "";
                checkBoxSOS_Good_APP.Checked = false;
                textBoxSOS_FBC.Text = "";
                textBoxSOS_FWCR.Text = "";
                checkBoxSOS_FWIR.Checked = false;
                textBoxSOS_CRC.Text = "";
            }
        }


        void parse_sos_message()
        {
            if (textBoxFA_Message.Text.Length >= 48)
            {
                string reasons = textBoxFA_Message.Text.Substring(32, 2);
                int reason = int.Parse(reasons, System.Globalization.NumberStyles.HexNumber);
                textBoxSOS_Reason.Text = reboot_reason(reason);
                string goodapps = textBoxFA_Message.Text.Substring(34, 2);
                int goodapp = int.Parse(goodapps, System.Globalization.NumberStyles.HexNumber);
                checkBoxSOS_Good_APP.Checked = goodapp != 0 ? true : false;
                string bwfs = textBoxFA_Message.Text.Substring(36, 4);
                int bwf = int.Parse(big_endian(bwfs), System.Globalization.NumberStyles.HexNumber);
                textBoxSOS_FBC.Text = Convert.ToString(bwf);
                string fwcrs = textBoxFA_Message.Text.Substring(40, 2);
                int fwcr = int.Parse(fwcrs, System.Globalization.NumberStyles.HexNumber);
                textBoxSOS_FWCR.Text = fw_copy_result(fwcr);
                string fwirs = textBoxFA_Message.Text.Substring(42, 2);
                int fwir = int.Parse(fwirs, System.Globalization.NumberStyles.HexNumber);
                checkBoxSOS_FWIR.Checked = fwir != 0 ? true : false;
                string crcs = textBoxFA_Message.Text.Substring(44, 4);
                textBoxSOS_CRC.Text = big_endian(crcs);
            }
            else
            {
                textBoxSOS_Reason.Text = "";
                checkBoxSOS_Good_APP.Checked = false;
                textBoxSOS_FBC.Text = "";
                textBoxSOS_FWCR.Text = "";
                checkBoxSOS_FWIR.Checked = false;
                textBoxSOS_CRC.Text = "";
            }
        }

        void parse_activated_message()
        {
            if (textBoxFA_Message.Text.Length >= 36)
            {
                string twvs = textBoxFA_Message.Text.Substring(32, 4);
                int twv = int.Parse(twvs, System.Globalization.NumberStyles.HexNumber);
                textBoxActiv_TWV.Text = Convert.ToString(twv) + " liters";
            }
            else
                textBoxActiv_TWV.Text = "";
        }

        void parse_gps_message()
        {
            if (textBoxFA_Message.Text.Length >= 64)
            {
                string hours = textBoxFA_Message.Text.Substring(32, 2);
                int hour = int.Parse(hours, System.Globalization.NumberStyles.HexNumber);
                textBoxGPS_Hours.Text = Convert.ToString(hour);
                string minutes = textBoxFA_Message.Text.Substring(34, 2);
                int minute = int.Parse(minutes, System.Globalization.NumberStyles.HexNumber);
                textBoxGPS_Minutes.Text = Convert.ToString(minute);

                textBoxGPS_Latitude.Text = format_latitude(textBoxFA_Message.Text.Substring(36, 8));
                textBoxGPS_Longitude.Text = format_longitude(textBoxFA_Message.Text.Substring(44, 8));

                string fixqs = textBoxFA_Message.Text.Substring(52, 2);
                int fixq = int.Parse(fixqs, System.Globalization.NumberStyles.HexNumber);
                checkBoxGPS_FixQ.Checked = fixq != 0 ? true : false;
                string sats = textBoxFA_Message.Text.Substring(54, 2);
                int sat = int.Parse(sats, System.Globalization.NumberStyles.HexNumber);
                textBoxGPS_Sat.Text = Convert.ToString(sat);
                string hdops = textBoxFA_Message.Text.Substring(56, 2);
                int hdop = int.Parse(hdops, System.Globalization.NumberStyles.HexNumber);
                hdops = Convert.ToString(hdop);
                if (hdops.Length == 2)
                    textBoxGPS_HDOP.Text = hdops.Substring(0, 1) + "." + hdops.Substring(1, 1) + " m";
                else if (hdops.Length == 3)
                    textBoxGPS_HDOP.Text = hdops.Substring(0, 2) + "." + hdops.Substring(2, 1) + " m";
                else
                    textBoxGPS_HDOP.Text = "0." + hdops.Substring(0, 1) + " m";
                string mts = textBoxFA_Message.Text.Substring(60, 4);
                int mt = int.Parse(mts, System.Globalization.NumberStyles.HexNumber);
                textBoxGPS_Time.Text = Convert.ToString(mt) + " sec";
            }
            else
            {
                textBoxGPS_Hours.Text = "";
                textBoxGPS_Minutes.Text = "";
                textBoxGPS_Latitude.Text = "";
                textBoxGPS_Longitude.Text = "";
                checkBoxGPS_FixQ.Checked = false;
                textBoxGPS_Sat.Text = "";
                textBoxGPS_HDOP.Text = "";
                textBoxGPS_Time.Text = "";
            }
        }

        void parse_daily_water_log_message()
        {
            int i;

            if (textBoxFA_Message.Text.Length >= 256)
            {
                int cursor = 32;

                textBoxCSVstring.Text = textBoxCSVwell.Text + "," + textBoxCSVmonth.Text + " " + textBoxCSVhour.Text + ",";

                // clear out the list view before filling again
                listViewDWL_LPH.Items.Clear();
                for (i = 0; i < 24; i++)
                {
                    string[] arr = new string[2];
                    string times;
                    ListViewItem itm;
                    int iph;

                    times = Convert.ToString(i);

                    textBoxCSVstring.Text = textBoxCSVstring.Text + "t:" + times + ",";

                    if (i == 0)
                        arr[0] = "12 am";
                    else if (i < 12)
                        arr[0] = times + " am";
                    else
                    {
                        if (i != 12)
                            times = Convert.ToString(i - 12);
                        arr[0] = times + " pm";
                    }
                    arr[1] = textBoxFA_Message.Text.Substring(cursor, 4);
                    iph = int.Parse(arr[1], System.Globalization.NumberStyles.HexNumber);
                    float flow = (float)iph * 32 / 1000;
                    // pour data is milliliters/32  scale it up
                    arr[1] = Convert.ToString(flow);
                    cursor += 4;
                    itm = new ListViewItem(arr);
                    listViewDWL_LPH.Items.Add(itm);

                    textBoxCSVstring.Text = textBoxCSVstring.Text + Convert.ToString(flow) + ",";
                }
                string totals = textBoxFA_Message.Text.Substring(128, 4);
                int total = int.Parse(totals, System.Globalization.NumberStyles.HexNumber);
                textBoxDWL_Total.Text = Convert.ToString(total);
                textBoxCSVstring.Text = textBoxCSVstring.Text + "tot:" + textBoxDWL_Total.Text + ",";

                string avgs = textBoxFA_Message.Text.Substring(132, 4);
                int avg = int.Parse(avgs, System.Globalization.NumberStyles.HexNumber);
                textBoxDWL_Average.Text = Convert.ToString(avg);
                textBoxCSVstring.Text = textBoxCSVstring.Text + "avg:" + textBoxDWL_Average.Text + ",";

                string rfs = textBoxFA_Message.Text.Substring(136, 2);
                int rf = int.Parse(rfs, System.Globalization.NumberStyles.HexNumber);
                checkBoxDWL_RedFlag.Checked = rf != 0 ? true : false;
                textBoxCSVstring.Text = textBoxCSVstring.Text + "red:" + Convert.ToString(checkBoxDWL_RedFlag.Checked) + ",";

                string unks = textBoxFA_Message.Text.Substring(140, 4);
                int unk = int.Parse(unks, System.Globalization.NumberStyles.HexNumber);
                textBoxDWL_Unknown.Text = Convert.ToString(unk);
                textBoxCSVstring.Text = textBoxCSVstring.Text + "unk:" + textBoxDWL_Unknown.Text + ",";

                string pad0s = textBoxFA_Message.Text.Substring(144, 4);
                int pad0 = int.Parse(pad0s, System.Globalization.NumberStyles.HexNumber);
                textBoxDWL_Pad0.Text = Convert.ToString(pad0);
                textBoxCSVstring.Text = textBoxCSVstring.Text + "p0:" + textBoxDWL_Pad0.Text + ",";
                string pad1s = textBoxFA_Message.Text.Substring(148, 4);
                int pad1 = int.Parse(pad1s, System.Globalization.NumberStyles.HexNumber);
                textBoxDWL_Pad1.Text = Convert.ToString(pad1);
                textBoxCSVstring.Text = textBoxCSVstring.Text + "p1:" + textBoxDWL_Pad1.Text + ",";
                string pad2s = textBoxFA_Message.Text.Substring(152, 4);
                int pad2 = int.Parse(pad2s, System.Globalization.NumberStyles.HexNumber);
                textBoxDWL_Pad2.Text = Convert.ToString(pad2);
                textBoxCSVstring.Text = textBoxCSVstring.Text + "p2:" + textBoxDWL_Pad2.Text + ",";
                string pad3s = textBoxFA_Message.Text.Substring(156, 4);
                int pad3 = int.Parse(pad3s, System.Globalization.NumberStyles.HexNumber);
                textBoxDWL_Pad3.Text = Convert.ToString(pad3);
                textBoxCSVstring.Text = textBoxCSVstring.Text + "p3:" + textBoxDWL_Pad3.Text + ",";
                string pad4s = textBoxFA_Message.Text.Substring(160, 4);
                int pad4 = int.Parse(pad4s, System.Globalization.NumberStyles.HexNumber);
                textBoxDWL_Pad4.Text = Convert.ToString(pad4);
                textBoxCSVstring.Text = textBoxCSVstring.Text + "p4:" + textBoxDWL_Pad4.Text + ",";
                string pad5s = textBoxFA_Message.Text.Substring(164, 4);
                int pad5 = int.Parse(pad5s, System.Globalization.NumberStyles.HexNumber);
                textBoxDWL_Pad5.Text = Convert.ToString(pad5);
                textBoxCSVstring.Text = textBoxCSVstring.Text + "p5:"+ textBoxDWL_Pad5.Text + ",";
                string reserveds = textBoxFA_Message.Text.Substring(168, 88); //168, 88
                checkBoxDWL_ResOK.Checked = OnlyFFInString(reserveds);
                textBoxCSVstring.Text = textBoxCSVstring.Text + "ff:" + Convert.ToString(checkBoxDWL_ResOK.Checked);
            }
            else
            {
                listViewDWL_LPH.Items.Clear();
                for (i = 0; i < 24; i++)
                {
                    string[] arr = new string[2];
                    string times;
                    ListViewItem itm;

                    times = Convert.ToString(i);
                    if (i == 0)
                        arr[0] = "12 am";
                    else if (i < 12)
                        arr[0] = times + " am";
                    else
                    {
                        if (i != 12)
                            times = Convert.ToString(i - 12);
                        arr[0] = times + " pm";
                    }
                    arr[1] = "";
                    itm = new ListViewItem(arr);
                    listViewDWL_LPH.Items.Add(itm);
                }

                textBoxDWL_Total.Text = "";
                textBoxDWL_Average.Text = "";
                checkBoxDWL_RedFlag.Checked = false;
                textBoxDWL_Unknown.Text = "";
                textBoxDWL_Pad0.Text = "";
                textBoxDWL_Pad1.Text = "";
                textBoxDWL_Pad2.Text = "";
                textBoxDWL_Pad3.Text = "";
                textBoxDWL_Pad4.Text = "";
                textBoxDWL_Pad5.Text = "";
            }
        }

        string big_endian(string num_in)
        {
            string answer = num_in.Substring(2, 2) + num_in.Substring(0, 2);
            return (answer);
        }


        string pad_state(int state_in)
        {
            string state_out;

            switch (state_in)
            {
                case 1:
                    state_out = "W";
                    break;
                case 2:
                    state_out = "w";
                    break;
                case 3:
                    state_out = "A";
                    break;
                case 4:
                    state_out = "a";
                    break;
                default:
                    state_out = "?";
                    break;
            }
            return (state_out);
        }

    public readonly int[] pad_drain_volume = { 161, 193, 199, 186, 168, 93 };

        int prop_percent(int target_air, int target_water, int mean)
        {
            
            int pad_diff = target_air - target_water - 450;
            int mean_diff = target_air - mean;

            if (mean_diff > 450 )
            {
                if (mean != target_water)
                {
                    mean_diff -= 450;
                    mean_diff *= 100;
                    mean_diff /= pad_diff;
                    // as the proportion of pad diff to mean diff grows, the coverage approaches 100%
                    mean_diff = mean_diff % 100;
                }
                else
                    mean_diff = 100;
            }
		    else
            {
                // no significant water measured over "top pad"
                mean_diff = 0;
            }

            return (mean_diff);
        }


        string midpoint(int target_air, int target_water, int mean, int state_in)
        {
            int target_width;
            int target_midpoint;
            int state = state_in;

            if (target_air > 0 && target_water> 0)
            {
                target_width = target_air - target_water;

                if (target_width > 450)
                {
                    target_midpoint = target_water + (target_width / 2);

                    if (mean >= target_midpoint)
                    {
                        if (target_air == mean)
                            state = 3;
                        else
                            state = 4;
                    }
                    else if (mean < target_midpoint)
                    {
                        if (target_water == mean)
                            state = 1;
                        else
                            state = 2;
                    }
                }
            }
            return (pad_state(state));
        }

        void parse_sensor_data_message()
        {
            string bl0s = textBoxFA_Message.Text.Substring(32, 4);
            int bl0 = int.Parse(big_endian(bl0s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDpadBL0.Text = Convert.ToString(bl0);
            string bl1s = textBoxFA_Message.Text.Substring(36, 4);
            int bl1 = int.Parse(big_endian(bl1s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDpadBL1.Text = Convert.ToString(bl1);
            string bl2s = textBoxFA_Message.Text.Substring(40, 4);
            int bl2 = int.Parse(big_endian(bl2s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDpadBL2.Text = Convert.ToString(bl2);
            string bl3s = textBoxFA_Message.Text.Substring(44, 4);
            int bl3 = int.Parse(big_endian(bl3s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDpadBL3.Text = Convert.ToString(bl3);
            string bl4s = textBoxFA_Message.Text.Substring(48, 4);
            int bl4 = int.Parse(big_endian(bl4s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDpadBL4.Text = Convert.ToString(bl4);
            string bl5s = textBoxFA_Message.Text.Substring(52, 4);
            int bl5 = int.Parse(big_endian(bl5s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDpadBL5.Text = Convert.ToString(bl5);
       
            string airdev0s = textBoxFA_Message.Text.Substring(56, 4);
            int airdev0 = int.Parse(big_endian(airdev0s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDairDev0.Text = Convert.ToString(airdev0);
            string airdev1s = textBoxFA_Message.Text.Substring(60, 4);
            int airdev1 = int.Parse(big_endian(airdev1s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDairDev1.Text = Convert.ToString(airdev1);
            string airdev2s = textBoxFA_Message.Text.Substring(64, 4);
            int airdev2 = int.Parse(big_endian(airdev2s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDairDev2.Text = Convert.ToString(airdev2);
            string airdev3s = textBoxFA_Message.Text.Substring(68, 4);
            int airdev3 = int.Parse(big_endian(airdev3s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDairDev3.Text = Convert.ToString(airdev3);
            string airdev4s = textBoxFA_Message.Text.Substring(72, 4);
            int airdev4 = int.Parse(big_endian(airdev4s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDairDev4.Text = Convert.ToString(airdev4);
            string airdev5s = textBoxFA_Message.Text.Substring(76, 4);
            int airdev5 = int.Parse(big_endian(airdev5s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDairDev5.Text = Convert.ToString(airdev5);

            string padTemps = textBoxFA_Message.Text.Substring(80, 4);
            int padTemp = int.Parse(big_endian(padTemps), System.Globalization.NumberStyles.HexNumber);
            textBoxSDblTemp.Text = Convert.ToString(padTemp);

            string currTemps = textBoxFA_Message.Text.Substring(84, 4);
            int currTemp = int.Parse(big_endian(currTemps), System.Globalization.NumberStyles.HexNumber);
            textBoxSDCurTemp.Text = Convert.ToString(currTemp);

            string seqUnks = textBoxFA_Message.Text.Substring(88, 4);
            int seqUnk = int.Parse(big_endian(seqUnks), System.Globalization.NumberStyles.HexNumber);
            textBoxSdseqUnk.Text = Convert.ToString(seqUnk);

            string last_means0 = textBoxFA_Message.Text.Substring(92, 4);
            string target_airs0 = textBoxFA_Message.Text.Substring(96, 4);
            string podtemp_airs0 = textBoxFA_Message.Text.Substring(100, 4);
            string target_waters0 = textBoxFA_Message.Text.Substring(104, 4);
            string podtemp_waters0 = textBoxFA_Message.Text.Substring(108, 4);
            string states0 = textBoxFA_Message.Text.Substring(112, 2);   //skipping uint8_t num_samp
            int last_mean0 = int.Parse(big_endian(last_means0), System.Globalization.NumberStyles.HexNumber);
            int target_air0 = int.Parse(big_endian(target_airs0), System.Globalization.NumberStyles.HexNumber);
            int podtemp_air0 = int.Parse(big_endian(podtemp_airs0), System.Globalization.NumberStyles.HexNumber);
            int target_water0 = int.Parse(big_endian(target_waters0), System.Globalization.NumberStyles.HexNumber);
            int podtemp_water0 = int.Parse(big_endian(podtemp_waters0), System.Globalization.NumberStyles.HexNumber);
            int state0 = int.Parse(states0, System.Globalization.NumberStyles.HexNumber);
            int pct0 = prop_percent(target_air0, target_water0, last_mean0);
            textBoxSDProp0.Text = Convert.ToString(pct0);
            textBoxSDmidpoint0.Text = midpoint(target_air0, target_water0, last_mean0, state0);

            textBoxSDlastMean0.Text = Convert.ToString(last_mean0);
            textBoxSDairTarg0.Text = Convert.ToString(target_air0);
            textBoxSDairTemp0.Text = Convert.ToString(podtemp_air0);
            textBoxSDwaterTarg0.Text = Convert.ToString(target_water0);
            textBoxSDwaterTemp0.Text = Convert.ToString(podtemp_water0);
            textBoxMarginMin0.Text = "n/a";
            textBoxMarginMax0.Text = "n/a";
            textBoxSDstate0.Text = pad_state(state0);

            string last_means1 = textBoxFA_Message.Text.Substring(116, 4);
            string target_airs1 = textBoxFA_Message.Text.Substring(120, 4);
            string podtemp_airs1 = textBoxFA_Message.Text.Substring(124, 4);
            string target_waters1 = textBoxFA_Message.Text.Substring(128, 4);
            string podtemp_waters1 = textBoxFA_Message.Text.Substring(132, 4);
            string states1 = textBoxFA_Message.Text.Substring(136, 2);   //skipping uint8_t num_samp
            int last_mean1 = int.Parse(big_endian(last_means1), System.Globalization.NumberStyles.HexNumber);
            int target_air1 = int.Parse(big_endian(target_airs1), System.Globalization.NumberStyles.HexNumber);
            int podtemp_air1 = int.Parse(big_endian(podtemp_airs1), System.Globalization.NumberStyles.HexNumber);
            int target_water1 = int.Parse(big_endian(target_waters1), System.Globalization.NumberStyles.HexNumber);
            int podtemp_water1 = int.Parse(big_endian(podtemp_waters1), System.Globalization.NumberStyles.HexNumber);
            int state1 = int.Parse(states1, System.Globalization.NumberStyles.HexNumber);
            int pct1 = prop_percent(target_air1, target_water1, last_mean1);
            textBoxSDProp1.Text = Convert.ToString(pct1);
            textBoxSDmidpoint1.Text = midpoint(target_air1, target_water1, last_mean1, state1);

            textBoxSDlastMean1.Text = Convert.ToString(last_mean1);
            textBoxSDairTarg1.Text = Convert.ToString(target_air1);
            textBoxSDairTemp1.Text = Convert.ToString(podtemp_air1);
            textBoxSDwaterTarg1.Text = Convert.ToString(target_water1);
            textBoxSDwaterTemp1.Text = Convert.ToString(podtemp_water1);
            textBoxMarginMin1.Text = "n/a";
            textBoxMarginMax1.Text = "n/a";
            textBoxSDstate1.Text = pad_state(state1);

            string last_means2 = textBoxFA_Message.Text.Substring(140, 4);
            string target_airs2 = textBoxFA_Message.Text.Substring(144, 4);
            string podtemp_airs2 = textBoxFA_Message.Text.Substring(148, 4);
            string target_waters2 = textBoxFA_Message.Text.Substring(152, 4);
            string podtemp_waters2 = textBoxFA_Message.Text.Substring(156, 4);
            string states2 = textBoxFA_Message.Text.Substring(160, 2);   //skipping uint8_t num_samp
            int last_mean2 = int.Parse(big_endian(last_means2), System.Globalization.NumberStyles.HexNumber);
            int target_air2 = int.Parse(big_endian(target_airs2), System.Globalization.NumberStyles.HexNumber);
            int podtemp_air2 = int.Parse(big_endian(podtemp_airs2), System.Globalization.NumberStyles.HexNumber);
            int target_water2 = int.Parse(big_endian(target_waters2), System.Globalization.NumberStyles.HexNumber);
            int podtemp_water2 = int.Parse(big_endian(podtemp_waters2), System.Globalization.NumberStyles.HexNumber);
            int state2 = int.Parse(states2, System.Globalization.NumberStyles.HexNumber);
            int pct2 = prop_percent(target_air2, target_water2, last_mean2);
            textBoxSDProp2.Text = Convert.ToString(pct2);
            textBoxSDmidpoint2.Text = midpoint(target_air2, target_water2, last_mean2, state2);

            textBoxSDlastMean2.Text = Convert.ToString(last_mean2);
            textBoxSDairTarg2.Text = Convert.ToString(target_air2);
            textBoxSDairTemp2.Text = Convert.ToString(podtemp_air2);
            textBoxSDwaterTarg2.Text = Convert.ToString(target_water2);
            textBoxSDwaterTemp2.Text = Convert.ToString(podtemp_water2);
            textBoxMarginMin2.Text = "n/a";
            textBoxMarginMax2.Text = "n/a";
            textBoxSDstate2.Text = pad_state(state2);

            string last_means3 = textBoxFA_Message.Text.Substring(164, 4);
            string target_airs3 = textBoxFA_Message.Text.Substring(168, 4);
            string podtemp_airs3 = textBoxFA_Message.Text.Substring(172, 4);
            string target_waters3 = textBoxFA_Message.Text.Substring(176, 4);
            string podtemp_waters3 = textBoxFA_Message.Text.Substring(180, 4);
            string states3 = textBoxFA_Message.Text.Substring(184, 2);   //skipping uint8_t num_samp
            int last_mean3 = int.Parse(big_endian(last_means3), System.Globalization.NumberStyles.HexNumber);
            int target_air3 = int.Parse(big_endian(target_airs3), System.Globalization.NumberStyles.HexNumber);
            int podtemp_air3 = int.Parse(big_endian(podtemp_airs3), System.Globalization.NumberStyles.HexNumber);
            int target_water3 = int.Parse(big_endian(target_waters3), System.Globalization.NumberStyles.HexNumber);
            int podtemp_water3 = int.Parse(big_endian(podtemp_waters3), System.Globalization.NumberStyles.HexNumber);
            int state3 = int.Parse(states3, System.Globalization.NumberStyles.HexNumber);
            int pct3 = prop_percent(target_air3, target_water3, last_mean3);
            textBoxSDProp3.Text = Convert.ToString(pct3);
            textBoxSDmidpoint3.Text = midpoint(target_air3, target_water3, last_mean3, state3);

            textBoxSDlastMean3.Text = Convert.ToString(last_mean3);
            textBoxSDairTarg3.Text = Convert.ToString(target_air3);
            textBoxSDairTemp3.Text = Convert.ToString(podtemp_air3);
            textBoxSDwaterTarg3.Text = Convert.ToString(target_water3);
            textBoxSDwaterTemp3.Text = Convert.ToString(podtemp_water3);
            textBoxMarginMin3.Text = "n/a";
            textBoxMarginMax3.Text = "n/a";
            textBoxSDstate3.Text = pad_state(state3);

            string last_means4 = textBoxFA_Message.Text.Substring(188, 4);
            string target_airs4 = textBoxFA_Message.Text.Substring(192, 4);
            string podtemp_airs4 = textBoxFA_Message.Text.Substring(196, 4);
            string target_waters4 = textBoxFA_Message.Text.Substring(200, 4);
            string podtemp_waters4 = textBoxFA_Message.Text.Substring(204, 4);
            string states4 = textBoxFA_Message.Text.Substring(208, 2);   //skipping uint8_t num_samp
            int last_mean4 = int.Parse(big_endian(last_means4), System.Globalization.NumberStyles.HexNumber);
            int target_air4 = int.Parse(big_endian(target_airs4), System.Globalization.NumberStyles.HexNumber);
            int podtemp_air4 = int.Parse(big_endian(podtemp_airs4), System.Globalization.NumberStyles.HexNumber);
            int target_water4 = int.Parse(big_endian(target_waters4), System.Globalization.NumberStyles.HexNumber);
            int podtemp_water4 = int.Parse(big_endian(podtemp_waters4), System.Globalization.NumberStyles.HexNumber);
            int state4 = int.Parse(states4, System.Globalization.NumberStyles.HexNumber);
            int pct4 = prop_percent(target_air4, target_water4, last_mean4);
            textBoxSDProp4.Text = Convert.ToString(pct4);
            textBoxSDmidpoint4.Text = midpoint(target_air4, target_water4, last_mean4, state4);

            textBoxSDlastMean4.Text = Convert.ToString(last_mean4);
            textBoxSDairTarg4.Text = Convert.ToString(target_air4);
            textBoxSDairTemp4.Text = Convert.ToString(podtemp_air4);
            textBoxSDwaterTarg4.Text = Convert.ToString(target_water4);
            textBoxSDwaterTemp4.Text = Convert.ToString(podtemp_water4);
            textBoxMarginMin4.Text = "n/a";
            textBoxMarginMax4.Text = "n/a";
            textBoxSDstate4.Text = pad_state(state4);


            string last_means5 = textBoxFA_Message.Text.Substring(212, 4);
            string target_airs5 = textBoxFA_Message.Text.Substring(216, 4);
            string podtemp_airs5 = textBoxFA_Message.Text.Substring(220, 4);
            string target_waters5 = textBoxFA_Message.Text.Substring(224, 4);
            string podtemp_waters5 = textBoxFA_Message.Text.Substring(228, 4);
            string states5 = textBoxFA_Message.Text.Substring(232, 2);   //skipping uint8_t num_samp
            int last_mean5 = int.Parse(big_endian(last_means5), System.Globalization.NumberStyles.HexNumber);
            int target_air5 = int.Parse(big_endian(target_airs5), System.Globalization.NumberStyles.HexNumber);
            int podtemp_air5 = int.Parse(big_endian(podtemp_airs5), System.Globalization.NumberStyles.HexNumber);
            int target_water5 = int.Parse(big_endian(target_waters5), System.Globalization.NumberStyles.HexNumber);
            int podtemp_water5 = int.Parse(big_endian(podtemp_waters5), System.Globalization.NumberStyles.HexNumber);
            int state5 = int.Parse(states5, System.Globalization.NumberStyles.HexNumber);
            int pct5 = prop_percent(target_air5, target_water5, last_mean5);
            textBoxSDProp5.Text = Convert.ToString(pct5);
            textBoxSDmidpoint5.Text = midpoint(target_air5, target_water5, last_mean5, state5);

            textBoxSDlastMean5.Text = Convert.ToString(last_mean5);
            textBoxSDairTarg5.Text = Convert.ToString(target_air5);
            textBoxSDairTemp5.Text = Convert.ToString(podtemp_air5);
            textBoxSDwaterTarg5.Text = Convert.ToString(target_water5);
            textBoxSDwaterTemp5.Text = Convert.ToString(podtemp_water5);
            textBoxMarginMin5.Text = "n/a";
            textBoxMarginMax5.Text = "n/a";
            textBoxSDstate5.Text = pad_state(state5);

            string unknown_limits = textBoxFA_Message.Text.Substring(236, 4);
            string total_flows = textBoxFA_Message.Text.Substring(240, 4);
            string downspout_rates = textBoxFA_Message.Text.Substring(244, 4);
            int unknown_limit = int.Parse(big_endian(unknown_limits), System.Globalization.NumberStyles.HexNumber);
            int total_flow = int.Parse(big_endian(total_flows), System.Globalization.NumberStyles.HexNumber);
            int downspout_rate = int.Parse(big_endian(downspout_rates), System.Globalization.NumberStyles.HexNumber);
            textBoxSDunkLim.Text = Convert.ToString(unknown_limit);
            textBoxSDtotalVol.Text = Convert.ToString(total_flow);
            textBoxSDdownSpout.Text = Convert.ToString(downspout_rate);

            string water_limits = textBoxFA_Message.Text.Substring(248, 4);
            string water_resets = textBoxFA_Message.Text.Substring(252, 4);
            string trickle_vols = "????";
            if (textBoxFA_Message.Text.Length >= 260)
                trickle_vols = textBoxFA_Message.Text.Substring(256, 4);

            int water_limit = int.Parse(big_endian(water_limits), System.Globalization.NumberStyles.HexNumber);
            int water_reset = int.Parse(big_endian(water_resets), System.Globalization.NumberStyles.HexNumber);
            int trickle_vol = 0;
            if (textBoxFA_Message.Text.Length >= 260)
                trickle_vol = int.Parse(big_endian(trickle_vols), System.Globalization.NumberStyles.HexNumber);

            textBoxSDWaterLim.Text = Convert.ToString(water_limit);
            textBoxSDwaterResets.Text = Convert.ToString(water_reset);
            textBoxSDtrickleVol.Text = Convert.ToString(trickle_vol);

            int cairdev0 = target_air0 - last_mean0;
            textBoxAirDev0.Text = Convert.ToString(cairdev0);
            int cairdev1 = target_air1 - last_mean1;
            textBoxAirDev1.Text = Convert.ToString(cairdev1);
            int cairdev2 = target_air2 - last_mean2;
            textBoxAirDev2.Text = Convert.ToString(cairdev2);
            int cairdev3 = target_air3 - last_mean3;
            textBoxAirDev3.Text = Convert.ToString(cairdev3);
            int cairdev4 = target_air4 - last_mean4;
            textBoxAirDev4.Text = Convert.ToString(cairdev4);
            int cairdev5 = target_air5 - last_mean5;
            textBoxAirDev5.Text = Convert.ToString(cairdev5);

            int cmargin0 = target_air0 - target_water0;
            textBoxMargin0.Text = Convert.ToString(cmargin0);
            int cmargin1 = target_air1 - target_water1;
            textBoxMargin1.Text = Convert.ToString(cmargin1);
            int cmargin2 = target_air2 - target_water2;
            textBoxMargin2.Text = Convert.ToString(cmargin2);
            int cmargin3 = target_air3 - target_water3;
            textBoxMargin3.Text = Convert.ToString(cmargin3);
            int cmargin4 = target_air4 - target_water4;
            textBoxMargin4.Text = Convert.ToString(cmargin4);
            int cmargin5 = target_air5 - target_water5;
            textBoxMargin5.Text = Convert.ToString(cmargin5);


            // build csv string for excel sheet analysis
            textBoxCSVstring.Text = textBoxCSVwell.Text + "," + textBoxCSVmonth.Text + " " + textBoxCSVhour.Text + ",";
            textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxSDairTarg0.Text + "," + textBoxSDairTarg1.Text + "," + textBoxSDairTarg2.Text + "," + textBoxSDairTarg3.Text + "," + textBoxSDairTarg4.Text + "," + textBoxSDairTarg5.Text + ",";
            textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxAirDev0.Text + ","+textBoxAirDev1.Text + ","+textBoxAirDev2.Text + ","+textBoxAirDev3.Text + ","+textBoxAirDev4.Text + ","+textBoxAirDev5.Text + ",";
            textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxSDlastMean0.Text + "," + textBoxSDlastMean1.Text + "," + textBoxSDlastMean2.Text + "," + textBoxSDlastMean3.Text + "," + textBoxSDlastMean4.Text + "," + textBoxSDlastMean5.Text + ",";
            textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxSDwaterTarg0.Text + "," + textBoxSDwaterTarg1.Text + "," + textBoxSDwaterTarg2.Text + "," + textBoxSDwaterTarg3.Text + "," + textBoxSDwaterTarg4.Text + "," + textBoxSDwaterTarg5.Text + ",";
            textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxMargin0.Text + "," + textBoxMargin1.Text + "," + textBoxMargin2.Text + "," + textBoxMargin3.Text + "," + textBoxMargin4.Text + "," + textBoxMargin5.Text + ",";
            if (textBoxSDtotalVol.Text.Length > 3)
                textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxSDtotalVol.Text.Substring(0, textBoxSDtotalVol.Text.Length - 3) + "." + textBoxSDtotalVol.Text.Substring(textBoxSDtotalVol.Text.Length - 3, 3) + ",";
            else if (textBoxSDtotalVol.Text.Length > 2)
                textBoxCSVstring.Text = textBoxCSVstring.Text + "0." + textBoxSDtotalVol.Text + ",";
            else if (textBoxSDtotalVol.Text.Length > 1)
                textBoxCSVstring.Text = textBoxCSVstring.Text + "0.0" + textBoxSDtotalVol.Text + ",";
            else if (textBoxSDtotalVol.Text.Length > 0)
                textBoxCSVstring.Text = textBoxCSVstring.Text + "0.00" + textBoxSDtotalVol.Text + ",";
            else
                textBoxCSVstring.Text = textBoxCSVstring.Text + "0.000" + ",";
            if (textBoxSDairTemp5.Text.Length > 1)
                textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxSDairTemp5.Text.Substring(0,textBoxSDairTemp5.Text.Length-1) + "." + textBoxSDairTemp5.Text.Substring(textBoxSDairTemp5.Text.Length - 1,1) + ",";
            else
                textBoxCSVstring.Text = textBoxCSVstring.Text + "0."+ textBoxSDairTemp5.Text + ",";

            textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxSDtrickleVol.Text;
        }

        void parse_new_sensor_data_message()
        {
            string bl0s = textBoxFA_Message.Text.Substring(32, 4);
            int bl0 = int.Parse(big_endian(bl0s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDpadBL0.Text = Convert.ToString(bl0);
            string bl1s = textBoxFA_Message.Text.Substring(36, 4);
            int bl1 = int.Parse(big_endian(bl1s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDpadBL1.Text = Convert.ToString(bl1);
            string bl2s = textBoxFA_Message.Text.Substring(40, 4);
            int bl2 = int.Parse(big_endian(bl2s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDpadBL2.Text = Convert.ToString(bl2);
            string bl3s = textBoxFA_Message.Text.Substring(44, 4);
            int bl3 = int.Parse(big_endian(bl3s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDpadBL3.Text = Convert.ToString(bl3);
            string bl4s = textBoxFA_Message.Text.Substring(48, 4);
            int bl4 = int.Parse(big_endian(bl4s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDpadBL4.Text = Convert.ToString(bl4);
            string bl5s = textBoxFA_Message.Text.Substring(52, 4);
            int bl5 = int.Parse(big_endian(bl5s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDpadBL5.Text = Convert.ToString(bl5);

            string airdev0s = textBoxFA_Message.Text.Substring(56, 4);
            int airdev0 = int.Parse(big_endian(airdev0s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDairDev0.Text = Convert.ToString(airdev0);
            string airdev1s = textBoxFA_Message.Text.Substring(60, 4);
            int airdev1 = int.Parse(big_endian(airdev1s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDairDev1.Text = Convert.ToString(airdev1);
            string airdev2s = textBoxFA_Message.Text.Substring(64, 4);
            int airdev2 = int.Parse(big_endian(airdev2s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDairDev2.Text = Convert.ToString(airdev2);
            string airdev3s = textBoxFA_Message.Text.Substring(68, 4);
            int airdev3 = int.Parse(big_endian(airdev3s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDairDev3.Text = Convert.ToString(airdev3);
            string airdev4s = textBoxFA_Message.Text.Substring(72, 4);
            int airdev4 = int.Parse(big_endian(airdev4s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDairDev4.Text = Convert.ToString(airdev4);
            string airdev5s = textBoxFA_Message.Text.Substring(76, 4);
            int airdev5 = int.Parse(big_endian(airdev5s), System.Globalization.NumberStyles.HexNumber);
            textBoxSDairDev5.Text = Convert.ToString(airdev5);

            string padTemps = textBoxFA_Message.Text.Substring(80, 4);
            int padTemp = int.Parse(big_endian(padTemps), System.Globalization.NumberStyles.HexNumber);
            textBoxSDblTemp.Text = Convert.ToString(padTemp);

            string currTemps = textBoxFA_Message.Text.Substring(84, 4);
            int currTemp = int.Parse(big_endian(currTemps), System.Globalization.NumberStyles.HexNumber);
            textBoxSDCurTemp.Text = Convert.ToString(currTemp);

            string seqUnks = textBoxFA_Message.Text.Substring(88, 4);
            int seqUnk = int.Parse(big_endian(seqUnks), System.Globalization.NumberStyles.HexNumber);
            textBoxSdseqUnk.Text = Convert.ToString(seqUnk);

            string last_means0 = textBoxFA_Message.Text.Substring(92, 4);
            string target_airs0 = textBoxFA_Message.Text.Substring(96, 4);
            string podtemp_airs0 = textBoxFA_Message.Text.Substring(100, 4);
            string target_waters0 = textBoxFA_Message.Text.Substring(104, 4);
            string podtemp_waters0 = textBoxFA_Message.Text.Substring(108, 4);
            string min_margs0 = textBoxFA_Message.Text.Substring(112, 4);
            string max_margs0 = textBoxFA_Message.Text.Substring(116, 4);
            string states0 = textBoxFA_Message.Text.Substring(120, 2);   //skipping uint8_t num_samp
            int last_mean0 = int.Parse(big_endian(last_means0), System.Globalization.NumberStyles.HexNumber);
            int target_air0 = int.Parse(big_endian(target_airs0), System.Globalization.NumberStyles.HexNumber);
            int podtemp_air0 = int.Parse(big_endian(podtemp_airs0), System.Globalization.NumberStyles.HexNumber);
            int target_water0 = int.Parse(big_endian(target_waters0), System.Globalization.NumberStyles.HexNumber);
            int podtemp_water0 = int.Parse(big_endian(podtemp_waters0), System.Globalization.NumberStyles.HexNumber);
            int min_marg0 = int.Parse(big_endian(min_margs0), System.Globalization.NumberStyles.HexNumber);
            int max_marg0 = int.Parse(big_endian(max_margs0), System.Globalization.NumberStyles.HexNumber);
            int state0 = int.Parse(states0, System.Globalization.NumberStyles.HexNumber);
            int pct0 = prop_percent(target_air0, target_water0, last_mean0);
            textBoxSDProp0.Text = Convert.ToString(pct0);
            textBoxSDmidpoint0.Text = midpoint(target_air0, target_water0, last_mean0, state0);

            textBoxSDlastMean0.Text = Convert.ToString(last_mean0);
            textBoxSDairTarg0.Text = Convert.ToString(target_air0);
            textBoxSDairTemp0.Text = Convert.ToString(podtemp_air0);
            textBoxSDwaterTarg0.Text = Convert.ToString(target_water0);
            textBoxSDwaterTemp0.Text = Convert.ToString(podtemp_water0);
            textBoxMarginMin0.Text = Convert.ToString(min_marg0);
            textBoxMarginMax0.Text = Convert.ToString(max_marg0);
            textBoxSDstate0.Text = pad_state(state0);

            string last_means1 = textBoxFA_Message.Text.Substring(124, 4);
            string target_airs1 = textBoxFA_Message.Text.Substring(128, 4);
            string podtemp_airs1 = textBoxFA_Message.Text.Substring(132, 4);
            string target_waters1 = textBoxFA_Message.Text.Substring(136, 4);
            string podtemp_waters1 = textBoxFA_Message.Text.Substring(140, 4);
            string min_margs1 = textBoxFA_Message.Text.Substring(144, 4);
            string max_margs1 = textBoxFA_Message.Text.Substring(148, 4);
            string states1 = textBoxFA_Message.Text.Substring(152, 2);   //skipping uint8_t num_samp
            int last_mean1 = int.Parse(big_endian(last_means1), System.Globalization.NumberStyles.HexNumber);
            int target_air1 = int.Parse(big_endian(target_airs1), System.Globalization.NumberStyles.HexNumber);
            int podtemp_air1 = int.Parse(big_endian(podtemp_airs1), System.Globalization.NumberStyles.HexNumber);
            int target_water1 = int.Parse(big_endian(target_waters1), System.Globalization.NumberStyles.HexNumber);
            int podtemp_water1 = int.Parse(big_endian(podtemp_waters1), System.Globalization.NumberStyles.HexNumber);
            int min_marg1 = int.Parse(big_endian(min_margs1), System.Globalization.NumberStyles.HexNumber);
            int max_marg1 = int.Parse(big_endian(max_margs1), System.Globalization.NumberStyles.HexNumber);
            int state1 = int.Parse(states1, System.Globalization.NumberStyles.HexNumber);
            int pct1 = prop_percent(target_air1, target_water1, last_mean1);
            textBoxSDProp1.Text = Convert.ToString(pct1);
            textBoxSDmidpoint1.Text = midpoint(target_air1, target_water1, last_mean1, state1);

            textBoxSDlastMean1.Text = Convert.ToString(last_mean1);
            textBoxSDairTarg1.Text = Convert.ToString(target_air1);
            textBoxSDairTemp1.Text = Convert.ToString(podtemp_air1);
            textBoxSDwaterTarg1.Text = Convert.ToString(target_water1);
            textBoxSDwaterTemp1.Text = Convert.ToString(podtemp_water1);
            textBoxMarginMin1.Text = Convert.ToString(min_marg1);
            textBoxMarginMax1.Text = Convert.ToString(max_marg1);
            textBoxSDstate1.Text = pad_state(state1);

            string last_means2 = textBoxFA_Message.Text.Substring(156, 4);
            string target_airs2 = textBoxFA_Message.Text.Substring(160, 4);
            string podtemp_airs2 = textBoxFA_Message.Text.Substring(164, 4);
            string target_waters2 = textBoxFA_Message.Text.Substring(168, 4);
            string podtemp_waters2 = textBoxFA_Message.Text.Substring(172, 4);
            string min_margs2 = textBoxFA_Message.Text.Substring(176, 4);
            string max_margs2 = textBoxFA_Message.Text.Substring(180, 4);
            string states2 = textBoxFA_Message.Text.Substring(184, 2);   //skipping uint8_t num_samp
            int last_mean2 = int.Parse(big_endian(last_means2), System.Globalization.NumberStyles.HexNumber);
            int target_air2 = int.Parse(big_endian(target_airs2), System.Globalization.NumberStyles.HexNumber);
            int podtemp_air2 = int.Parse(big_endian(podtemp_airs2), System.Globalization.NumberStyles.HexNumber);
            int target_water2 = int.Parse(big_endian(target_waters2), System.Globalization.NumberStyles.HexNumber);
            int podtemp_water2 = int.Parse(big_endian(podtemp_waters2), System.Globalization.NumberStyles.HexNumber);
            int min_marg2 = int.Parse(big_endian(min_margs2), System.Globalization.NumberStyles.HexNumber);
            int max_marg2 = int.Parse(big_endian(max_margs2), System.Globalization.NumberStyles.HexNumber);
            int state2 = int.Parse(states2, System.Globalization.NumberStyles.HexNumber);
            int pct2 = prop_percent(target_air2, target_water2, last_mean2);
            textBoxSDProp2.Text = Convert.ToString(pct2);
            textBoxSDmidpoint2.Text = midpoint(target_air2, target_water2, last_mean2, state2);

            textBoxSDlastMean2.Text = Convert.ToString(last_mean2);
            textBoxSDairTarg2.Text = Convert.ToString(target_air2);
            textBoxSDairTemp2.Text = Convert.ToString(podtemp_air2);
            textBoxSDwaterTarg2.Text = Convert.ToString(target_water2);
            textBoxSDwaterTemp2.Text = Convert.ToString(podtemp_water2);
            textBoxMarginMin2.Text = Convert.ToString(min_marg2);
            textBoxMarginMax2.Text = Convert.ToString(max_marg2);
            textBoxSDstate2.Text = pad_state(state2);

            string last_means3 = textBoxFA_Message.Text.Substring(188, 4);
            string target_airs3 = textBoxFA_Message.Text.Substring(192, 4);
            string podtemp_airs3 = textBoxFA_Message.Text.Substring(196, 4);
            string target_waters3 = textBoxFA_Message.Text.Substring(200, 4);
            string podtemp_waters3 = textBoxFA_Message.Text.Substring(204, 4);
            string min_margs3 = textBoxFA_Message.Text.Substring(208, 4);
            string max_margs3 = textBoxFA_Message.Text.Substring(212, 4);
            string states3 = textBoxFA_Message.Text.Substring(216, 2);   //skipping uint8_t num_samp
            int last_mean3 = int.Parse(big_endian(last_means3), System.Globalization.NumberStyles.HexNumber);
            int target_air3 = int.Parse(big_endian(target_airs3), System.Globalization.NumberStyles.HexNumber);
            int podtemp_air3 = int.Parse(big_endian(podtemp_airs3), System.Globalization.NumberStyles.HexNumber);
            int target_water3 = int.Parse(big_endian(target_waters3), System.Globalization.NumberStyles.HexNumber);
            int podtemp_water3 = int.Parse(big_endian(podtemp_waters3), System.Globalization.NumberStyles.HexNumber);
            int min_marg3 = int.Parse(big_endian(min_margs3), System.Globalization.NumberStyles.HexNumber);
            int max_marg3 = int.Parse(big_endian(max_margs3), System.Globalization.NumberStyles.HexNumber);
            int state3 = int.Parse(states3, System.Globalization.NumberStyles.HexNumber);
            int pct3 = prop_percent(target_air3, target_water3, last_mean3);
            textBoxSDProp3.Text = Convert.ToString(pct3);
            textBoxSDmidpoint3.Text = midpoint(target_air3, target_water3, last_mean3, state3);

            textBoxSDlastMean3.Text = Convert.ToString(last_mean3);
            textBoxSDairTarg3.Text = Convert.ToString(target_air3);
            textBoxSDairTemp3.Text = Convert.ToString(podtemp_air3);
            textBoxSDwaterTarg3.Text = Convert.ToString(target_water3);
            textBoxSDwaterTemp3.Text = Convert.ToString(podtemp_water3);
            textBoxMarginMin3.Text = Convert.ToString(min_marg3);
            textBoxMarginMax3.Text = Convert.ToString(max_marg3);
            textBoxSDstate3.Text = pad_state(state3);

            string last_means4 = textBoxFA_Message.Text.Substring(220, 4);
            string target_airs4 = textBoxFA_Message.Text.Substring(224, 4);
            string podtemp_airs4 = textBoxFA_Message.Text.Substring(228, 4);
            string target_waters4 = textBoxFA_Message.Text.Substring(232, 4);
            string podtemp_waters4 = textBoxFA_Message.Text.Substring(236, 4);
            string min_margs4 = textBoxFA_Message.Text.Substring(240, 4);
            string max_margs4 = textBoxFA_Message.Text.Substring(244, 4);
            string states4 = textBoxFA_Message.Text.Substring(248, 2);   //skipping uint8_t num_samp
            int last_mean4 = int.Parse(big_endian(last_means4), System.Globalization.NumberStyles.HexNumber);
            int target_air4 = int.Parse(big_endian(target_airs4), System.Globalization.NumberStyles.HexNumber);
            int podtemp_air4 = int.Parse(big_endian(podtemp_airs4), System.Globalization.NumberStyles.HexNumber);
            int target_water4 = int.Parse(big_endian(target_waters4), System.Globalization.NumberStyles.HexNumber);
            int podtemp_water4 = int.Parse(big_endian(podtemp_waters4), System.Globalization.NumberStyles.HexNumber);
            int min_marg4 = int.Parse(big_endian(min_margs4), System.Globalization.NumberStyles.HexNumber);
            int max_marg4 = int.Parse(big_endian(max_margs4), System.Globalization.NumberStyles.HexNumber);
            int state4 = int.Parse(states4, System.Globalization.NumberStyles.HexNumber);
            int pct4 = prop_percent(target_air4, target_water4, last_mean4);
            textBoxSDProp4.Text = Convert.ToString(pct4);
            textBoxSDmidpoint4.Text = midpoint(target_air4, target_water4, last_mean4, state4);

            textBoxSDlastMean4.Text = Convert.ToString(last_mean4);
            textBoxSDairTarg4.Text = Convert.ToString(target_air4);
            textBoxSDairTemp4.Text = Convert.ToString(podtemp_air4);
            textBoxSDwaterTarg4.Text = Convert.ToString(target_water4);
            textBoxSDwaterTemp4.Text = Convert.ToString(podtemp_water4);
            textBoxMarginMin4.Text = Convert.ToString(min_marg4);
            textBoxMarginMax4.Text = Convert.ToString(max_marg4);
            textBoxSDstate4.Text = pad_state(state4);

            string last_means5 = textBoxFA_Message.Text.Substring(252, 4);
            string target_airs5 = textBoxFA_Message.Text.Substring(256, 4);
            string podtemp_airs5 = textBoxFA_Message.Text.Substring(260, 4);
            string target_waters5 = textBoxFA_Message.Text.Substring(264, 4);
            string podtemp_waters5 = textBoxFA_Message.Text.Substring(268, 4);
            string min_margs5 = textBoxFA_Message.Text.Substring(272, 4);
            string max_margs5 = textBoxFA_Message.Text.Substring(276, 4);
            string states5 = textBoxFA_Message.Text.Substring(280, 2);   //skipping uint8_t num_samp
            int last_mean5 = int.Parse(big_endian(last_means5), System.Globalization.NumberStyles.HexNumber);
            int target_air5 = int.Parse(big_endian(target_airs5), System.Globalization.NumberStyles.HexNumber);
            int podtemp_air5 = int.Parse(big_endian(podtemp_airs5), System.Globalization.NumberStyles.HexNumber);
            int target_water5 = int.Parse(big_endian(target_waters5), System.Globalization.NumberStyles.HexNumber);
            int podtemp_water5 = int.Parse(big_endian(podtemp_waters5), System.Globalization.NumberStyles.HexNumber);
            int min_marg5 = int.Parse(big_endian(min_margs5), System.Globalization.NumberStyles.HexNumber);
            int max_marg5 = int.Parse(big_endian(max_margs5), System.Globalization.NumberStyles.HexNumber);
            int state5 = int.Parse(states5, System.Globalization.NumberStyles.HexNumber);
            int pct5 = prop_percent(target_air5, target_water5, last_mean5);
            textBoxSDProp5.Text = Convert.ToString(pct5);
            textBoxSDmidpoint5.Text = midpoint(target_air5, target_water5, last_mean5, state5);

            textBoxSDlastMean5.Text = Convert.ToString(last_mean5);
            textBoxSDairTarg5.Text = Convert.ToString(target_air5);
            textBoxSDairTemp5.Text = Convert.ToString(podtemp_air5);
            textBoxSDwaterTarg5.Text = Convert.ToString(target_water5);
            textBoxSDwaterTemp5.Text = Convert.ToString(podtemp_water5);
            textBoxMarginMin5.Text = Convert.ToString(min_marg5);
            textBoxMarginMax5.Text = Convert.ToString(max_marg5);
            textBoxSDstate5.Text = pad_state(state5);

            string unknown_limits = textBoxFA_Message.Text.Substring(284, 4);
            string total_flows = textBoxFA_Message.Text.Substring(288, 4);
            string downspout_rates = textBoxFA_Message.Text.Substring(292, 4);
            int unknown_limit = int.Parse(big_endian(unknown_limits), System.Globalization.NumberStyles.HexNumber);
            int total_flow = int.Parse(big_endian(total_flows), System.Globalization.NumberStyles.HexNumber);
            int downspout_rate = int.Parse(big_endian(downspout_rates), System.Globalization.NumberStyles.HexNumber);
            textBoxSDunkLim.Text = Convert.ToString(unknown_limit);
            textBoxSDtotalVol.Text = Convert.ToString(total_flow);
            textBoxSDdownSpout.Text = Convert.ToString(downspout_rate);

            string water_limits = textBoxFA_Message.Text.Substring(296, 4);
            string water_resets = textBoxFA_Message.Text.Substring(300, 4);
            string trickle_vols = "????";

            trickle_vols = textBoxFA_Message.Text.Substring(304, 4);
            string marginscores = "????";
            marginscores = textBoxFA_Message.Text.Substring(308, 4);

            int water_limit = int.Parse(big_endian(water_limits), System.Globalization.NumberStyles.HexNumber);
            int water_reset = int.Parse(big_endian(water_resets), System.Globalization.NumberStyles.HexNumber);
            int trickle_vol = 0;
                trickle_vol = int.Parse(big_endian(trickle_vols), System.Globalization.NumberStyles.HexNumber);
            int marginscore = 0;
                marginscore = int.Parse(big_endian(marginscores), System.Globalization.NumberStyles.HexNumber);

            textBoxSDWaterLim.Text = Convert.ToString(water_limit);
            textBoxSDwaterResets.Text = Convert.ToString(water_reset);
            textBoxSDtrickleVol.Text = Convert.ToString(trickle_vol);
            textBoxSDMargin.Text = Convert.ToString(marginscore);

            int cairdev0 = target_air0 - last_mean0;
            textBoxAirDev0.Text = Convert.ToString(cairdev0);
            int cairdev1 = target_air1 - last_mean1;
            textBoxAirDev1.Text = Convert.ToString(cairdev1);
            int cairdev2 = target_air2 - last_mean2;
            textBoxAirDev2.Text = Convert.ToString(cairdev2);
            int cairdev3 = target_air3 - last_mean3;
            textBoxAirDev3.Text = Convert.ToString(cairdev3);
            int cairdev4 = target_air4 - last_mean4;
            textBoxAirDev4.Text = Convert.ToString(cairdev4);
            int cairdev5 = target_air5 - last_mean5;
            textBoxAirDev5.Text = Convert.ToString(cairdev5);

            int cmargin0 = target_air0 - target_water0;
            textBoxMargin0.Text = Convert.ToString(cmargin0);
            int cmargin1 = target_air1 - target_water1;
            textBoxMargin1.Text = Convert.ToString(cmargin1);
            int cmargin2 = target_air2 - target_water2;
            textBoxMargin2.Text = Convert.ToString(cmargin2);
            int cmargin3 = target_air3 - target_water3;
            textBoxMargin3.Text = Convert.ToString(cmargin3);
            int cmargin4 = target_air4 - target_water4;
            textBoxMargin4.Text = Convert.ToString(cmargin4);
            int cmargin5 = target_air5 - target_water5;
            textBoxMargin5.Text = Convert.ToString(cmargin5);


            // build csv string for excel sheet analysis
            textBoxCSVstring.Text = textBoxCSVwell.Text + "," + textBoxCSVmonth.Text + " " + textBoxCSVhour.Text + ",";
            textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxSDairTarg0.Text + "," + textBoxSDairTarg1.Text + "," + textBoxSDairTarg2.Text + "," + textBoxSDairTarg3.Text + "," + textBoxSDairTarg4.Text + "," + textBoxSDairTarg5.Text + ",";
            textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxAirDev0.Text + "," + textBoxAirDev1.Text + "," + textBoxAirDev2.Text + "," + textBoxAirDev3.Text + "," + textBoxAirDev4.Text + "," + textBoxAirDev5.Text + ",";
            textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxSDlastMean0.Text + "," + textBoxSDlastMean1.Text + "," + textBoxSDlastMean2.Text + "," + textBoxSDlastMean3.Text + "," + textBoxSDlastMean4.Text + "," + textBoxSDlastMean5.Text + ",";
            textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxSDwaterTarg0.Text + "," + textBoxSDwaterTarg1.Text + "," + textBoxSDwaterTarg2.Text + "," + textBoxSDwaterTarg3.Text + "," + textBoxSDwaterTarg4.Text + "," + textBoxSDwaterTarg5.Text + ",";
            textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxMargin0.Text + "," + textBoxMargin1.Text + "," + textBoxMargin2.Text + "," + textBoxMargin3.Text + "," + textBoxMargin4.Text + "," + textBoxMargin5.Text + ",";
            if (textBoxSDtotalVol.Text.Length > 3)
                textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxSDtotalVol.Text.Substring(0, textBoxSDtotalVol.Text.Length - 3) + "." + textBoxSDtotalVol.Text.Substring(textBoxSDtotalVol.Text.Length - 3, 3) + ",";
            else if (textBoxSDtotalVol.Text.Length > 2)
                textBoxCSVstring.Text = textBoxCSVstring.Text + "0." + textBoxSDtotalVol.Text + ",";
            else if (textBoxSDtotalVol.Text.Length > 1)
                textBoxCSVstring.Text = textBoxCSVstring.Text + "0.0" + textBoxSDtotalVol.Text + ",";
            else if (textBoxSDtotalVol.Text.Length > 0)
                textBoxCSVstring.Text = textBoxCSVstring.Text + "0.00" + textBoxSDtotalVol.Text + ",";
            else
                textBoxCSVstring.Text = textBoxCSVstring.Text + "0.000" + ",";
            if (textBoxSDairTemp5.Text.Length > 1)
                textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxSDairTemp5.Text.Substring(0, textBoxSDairTemp5.Text.Length - 1) + "." + textBoxSDairTemp5.Text.Substring(textBoxSDairTemp5.Text.Length - 1, 1) + ",";
            else
                textBoxCSVstring.Text = textBoxCSVstring.Text + "0." + textBoxSDairTemp5.Text + ",";

            textBoxCSVstring.Text = textBoxCSVstring.Text + textBoxSDtrickleVol.Text;
        }

        int parse_common_message_header()
        {
            string msg_types;
            int msg_type = 0;

            if (textBoxFA_Message.Text.Length >= 4)
            {
                msg_types = textBoxFA_Message.Text.Substring(2, 2);
                msg_type = int.Parse(msg_types, System.Globalization.NumberStyles.HexNumber);
                textBoxGSM_MID.Text = message_id(msg_type);
            }

            if (textBoxFA_Message.Text.Length >= 17)
            {
                if (msg_type != 6)
                {
                    textBoxGMT_Time.Text = Convert.ToString(get_FA_time(textBoxFA_Message.Text.Substring(6, 12)));
                    textBoxLocal_Time.Text = Convert.ToString(get_FA_time(textBoxFA_Message.Text.Substring(6, 12)).AddHours(Convert.ToByte(textBoxTZ_Hours.Text)));
                }
                else
                {
                    textBoxGMT_Time.Text = "";
                    textBoxLocal_Time.Text = "";
                }
            }
            else
            {
                textBoxGMT_Time.Text = "";
                textBoxLocal_Time.Text = "";
            }
            if (textBoxFA_Message.Text.Length >= 6)
            {
                string pids = textBoxFA_Message.Text.Substring(4, 2);
                int pid = int.Parse(pids, System.Globalization.NumberStyles.HexNumber);
                if (pid == 3)
                    textBoxGSM_PID.Text = "Afridev2";
                else
                    textBoxGSM_PID.Text = "Unsupported";
            }
            else
                textBoxGSM_PID.Text = "";

            if (textBoxFA_Message.Text.Length >= 22)
            {
                string majors = textBoxFA_Message.Text.Substring(18, 2);
                string minors = textBoxFA_Message.Text.Substring(20, 2);
                int major = int.Parse(majors, System.Globalization.NumberStyles.HexNumber);
                int minor = int.Parse(minors, System.Globalization.NumberStyles.HexNumber);
                textBoxGSM_Version.Text = Convert.ToString(major) + "." + Convert.ToString(minor);
            }
            else
                textBoxGSM_Version.Text = "";

            if (textBoxFA_Message.Text.Length >= 26 && msg_type != 6)
            {
                string daysactives = textBoxFA_Message.Text.Substring(22, 4);
                int daysactive = int.Parse(daysactives, System.Globalization.NumberStyles.HexNumber);
                textBoxGSM_DaysActive.Text = Convert.ToString(daysactive);
            }
            else
            {
                textBoxGSM_DaysActive.Text = "";
            }
            if (textBoxFA_Message.Text.Length >= 30 && msg_type != 6)
            {
                string weeks = textBoxFA_Message.Text.Substring(26, 2);
                string days = textBoxFA_Message.Text.Substring(28, 2);
                int week = int.Parse(weeks, System.Globalization.NumberStyles.HexNumber);
                int day = int.Parse(days, System.Globalization.NumberStyles.HexNumber);
                textBoxGSM_Week.Text = Convert.ToString(week);
                textBoxGSM_Day.Text = Convert.ToString(day);
            }
            else
            {
                textBoxGSM_Week.Text = "";
                textBoxGSM_Day.Text = "";
            }
            return (msg_type);
        }


        private void textBoxFA_Message_TextChanged(object sender, EventArgs e)
        {
            int msg_type = 0;

            // Validate Incoming message
            if (textBoxFA_Message.Text.Length > 0)
            {
                textBoxFA_Message.Text = textBoxFA_Message.Text.Replace(" ", string.Empty);
            }

            if (OnlyHexInString(textBoxFA_Message.Text) && textBoxFA_Message.Text.Length > 0)
            {
                msg_type = parse_common_message_header();

                switch (msg_type)
                {
                    case 3: //OTA reply
                        parse_ota_reply_message();
                        break;
                    case 6: //sos message
                    case 35:
                        parse_sos_message();
                        break;
                    case 36:
                        parse_timestamp_message();
                        break;
                    case 7: // activated
                        parse_activated_message();
                        break;
                    case 8: // gps location
                        parse_gps_message();
                        break;
                    case 33: // daily water log
                        parse_daily_water_log_message();
                        break;
                    case 34: // sensor data
                        if (textBoxFA_Message.Text.Length >= 312)
                            parse_new_sensor_data_message();
                        else
                            parse_sensor_data_message();
                        break;
                }
            }
            this.Refresh();
        }

        private void Button_Calculate_Click(object sender, EventArgs e)
        {
            DateTime dt1 = DateTime.Now.ToUniversalTime();

            if (textBoxFA_Message.Text.Length >= 17)
            {
                DateTime fa = get_FA_time(textBoxFA_Message.Text.Substring(6, 12));
                TimeSpan span = dt1.Subtract(fa);

                // see http://www.ti.com/lit/an/slaa290a/slaa290a.pdf  The firmware code uses 
                // the TI RTC library
                
                int seconds = span.Seconds;
                int minutes = span.Minutes;
                int hours = span.Hours;
                int days = span.Days;

                // adjustments only go forwards
                if (seconds < 0)
                    seconds = 60 + seconds;

                if (minutes < 0)
                    minutes = 60 + minutes;

                if (hours < 0)
                    hours = 24 + hours;

                // days can't go backwards
                if (days < 0)
                    days = 0;

                // convert to hex
                string second = seconds.ToString("X2");
                string minute = minutes.ToString("X2");
                string hour = hours.ToString("X2");
                string day = days.ToString("X4");

                textBoxGMT_Message.Text = space_bytes("015AA5" + second + minute + hour + day);
            }
            this.Refresh();
        }

        private void buttonSetStorageClock_Click(object sender, EventArgs e)
        {
            DateTime dt1 = DateTime.Now.ToUniversalTime();
            DateTime lt1;
            int hours;
            int minutes;

            if (textBoxTZ_Minutes.Text.Length > 0 && textBoxTZ_Hours.Text.Length > 0 && textBoxTZ_Hours.Text != "-")
            {
                minutes = int.Parse(textBoxTZ_Minutes.Text);
                hours = int.Parse(textBoxTZ_Hours.Text);
 
                if (hours > 23 || hours < -23)
                    hours = 0;
                lt1 = dt1.AddHours(hours);

                if (minutes != 0)
                    minutes = 30;
                lt1 = lt1.AddMinutes(minutes);

                textBoxLocal_Time.Text = Convert.ToString(lt1);

                if (hours < 0)
                    hours = 24 + hours;

                textBoxTZ_Hours.Text = Convert.ToString(hours);
                textBoxTZ_Minutes.Text = Convert.ToString(minutes);

                string second = "00";
                string minute = minutes.ToString("X2");
                string hour = hours.ToString("X2");
                textBoxStorage_Message.Text = space_bytes("025AA5" + second + minute + hour);
                this.Refresh();
            }
        }

        private void textBoxTransRateDays_TextChanged(object sender, EventArgs e)
        {
            int days = int.Parse(textBoxTransRateDays.Text);

            if (days < 0)
                days = 0;
            if (days > 42)
                days = 42;

            string day = days.ToString("X2");
            textBoxTransRate_Message.Text = space_bytes("075AA5" + day);
            this.Refresh();
        }

        private void checkBoxNewGPS_CheckedChanged(object sender, EventArgs e)
        {
#if false
            if (checkBoxNewGPS.Checked)
            {
                checkBoxExistingGPS.Checked = false;
                textBoxGPSRequest.Text = space_bytes("0D5AA501");
            }
            else
            {
                checkBoxExistingGPS.Checked = true;
                textBoxGPSRequest.Text = space_bytes("0D5AA500");
            }
#endif
            this.Refresh();
        }

        private void checkBoxExistingGPS_CheckedChanged(object sender, EventArgs e)
        {
#if false
            if (checkBoxExistingGPS.Checked)
                checkBoxNewGPS.Checked = false;
            else
                checkBoxNewGPS.Checked = true;
#endif
            this.Refresh();
        }

        private void buttonGPS_Criteria_Click(object sender, EventArgs e)
        {
#if false
            int Satellite = int.Parse(textBoxSatellite.Text);
            int HDOP = int.Parse(textBoxHDOP.Text);
            int MeasTime = int.Parse(textBoxMeasTime.Text);
            if (Satellite < 4)
                Satellite = 4;
            if (Satellite > 8)
                Satellite = 8;
            if (HDOP < 10)
                HDOP = 10;
            if (HDOP > 200)
                HDOP = 200;
            if (MeasTime < 0)
                MeasTime = 0;
            if (MeasTime > 600)
                MeasTime = 600;

            string Satellites = Satellite.ToString("X2");
            string HDOPs = HDOP.ToString("X2");
            string MeasTimes = MeasTime.ToString("X4");

            textBoxGPSMC_Message.Text = space_bytes("0E5AA5" + Satellites + HDOPs + MeasTimes);
#endif
            this.Refresh();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            textBoxFA_Message.Text = "";
            this.Refresh();
        }

        private void groupBox13_Enter(object sender, EventArgs e)
        {

        }

        private void comboBoxSensorReq_SelectedIndexChanged(object sender, EventArgs e)
        {

            int UnkLmit = int.Parse(textBoxUnkLimit.Text);
            int WtrLmit = int.Parse(textBoxWaterLimit.Text);

            switch(comboBoxSensorReq.SelectedIndex)
            {
                case 0: textBoxSensorData.Text = space_bytes("0F5AA5000000"); break;
                case 1: textBoxSensorData.Text = space_bytes("0F5AA5010000"); break;
                case 2: textBoxSensorData.Text = space_bytes("0F5AA5020000"); break;
                case 3: textBoxSensorData.Text = space_bytes("0F5AA503"+ big_endian(UnkLmit.ToString("X4"))); break;
                case 4: textBoxSensorData.Text = space_bytes("0F5AA5040000"); break;
                case 5: textBoxSensorData.Text = space_bytes("0F5AA5050000"); break;
                case 6: textBoxSensorData.Text = space_bytes("0F5AA506" + big_endian(WtrLmit.ToString("X4"))); break;
                default: break;
            }
        }

        private void ButtonCSVRefresh_Click(object sender, EventArgs e)
        {
            if (textBoxFA_Message.Text.Length >= 312)
                parse_new_sensor_data_message();
            else
                parse_sensor_data_message();
 
            this.Refresh();
        }

        private void TextBoxSDairDev0_TextChanged(object sender, EventArgs e)
        {

        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        private void button2_Click(object sender, EventArgs e)
        {
            parse_daily_water_log_message();
            this.Refresh();
        }

        private void label26_Click(object sender, EventArgs e)
        {

        }

        private void label28_Click(object sender, EventArgs e)
        {

        }
    }
}