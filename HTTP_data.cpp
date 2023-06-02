
/* we use TEXTAREA: copy first part, add variable part, append second part */
const char data_html_a[] /*__attribute__((aligned(4)))*/ =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\r\n\
<html xmlns:v=\"urn:schemas-microsoft-com:vml\" xmlns:o=\"urn:schemas-microsoft-com:office:office\" xmlns:w=\"urn:schemas-microsoft-com:office:word\" xmlns=\"http://www.w3.org/TR/REC-html40\">\r\n\
<head>\r\n\
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\r\n\
<title>TeensySPIder</title>\r\n\
<style>\r\n\
.bg1 {background-color: rgb(180, 180, 180);}\r\n\
.bg2 {background-color: rgb(200, 200, 200);}\r\n\
.bg3 {font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(230, 230, 230); text-align: center;}\r\n\
.bg4 {font-family: Verdana; font-weight: bold; font-style: italic; text-align: center;}\r\n\
</style>\r\n\
</head>\r\n\
<body lang=\"EN-US\" link=\"blue\" vlink=\"blue\" bgcolor=\"#336666\">\r\n\
<table style=\"width: 860px; height: 30px;\" border=\"0\" cellpadding=\"2\" cellspacing=\"2\">\r\n\
<tbody>\r\n\
<tr>\r\n\
<td class=\"bg4\" style=\"background-color: rgb(40, 40, 255);\"><small><a href=\"/\"><span style=\"color: white;\">Home Page</span></a></small></td>\r\n\
<td class=\"bg4\" style=\"background-color: rgb(40, 40, 255);\"><small><a href=\"/\"><span style=\"color: white;\">List of Tasks</span></a></small></td>\r\n\
</tr>\r\n\
<tr>\r\n\
<td class=\"bg4\" style=\"background-color: rgb(60, 160, 255);\"><strong><a href=\"/\" style=\"text-decoration:none;\"><span style=\"color: white;\">Soft reset</span></a></strong></td>\r\n\
<td class=\"bg4\" style=\"background-color: rgb(200, 100, 255);\"><strong><a href=\"/\" style=\"text-decoration:none;\"><span style=\"color: white;\">Hard reset</span></a></strong></td>\r\n\
</tr>\r\n\
<tr class=\"bg2\">\r\n\
<td colspan=\"2\">\r\n\
<form action=\"/\">\r\n\
<input type=\"text\" name=\"CMD\" value=\"\" size=\"145\">\r\n\
</form>\r\n\
</td>\r\n\
</tr>\r\n\
<tr class=\"bg1\">\r\n\
<td colspan=\"2\">\r\n\
<form action=\"/\">\r\n\
<textarea name=\"CMD_RES\" rows=\"30\" cols=\"140\">\r\n"
#if 0
"</textarea>\r\n\
</form>\r\n\
</td>\r\n\
</tr>\r\n\
</tbody>\r\n\
</table>\r\n\
</body>\r\n\
</html>\r\n"
#endif
;

const char data_html_b[] /*__attribute__((aligned(4)))*/ =
"</textarea>\r\n\
</form>\r\n\
</td>\r\n\
</tr>\r\n\
</tbody>\r\n\
</table>\r\n\
</body>\r\n\
</html>\r\n"
;

