import smtplib
import sys
import argparse
from os.path import isfile, join, basename, splitext
from os import listdir
from email.mime.application import MIMEApplication
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText


def get_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('to_mail', action="store", help="Email address to which the message is sent")
    parser.add_argument('from_mail', action="store", help="Email address that the message is sent from")
    parser.add_argument('from_pass', action="store", help="Password for the email address that the message is sent from")
    parser.add_argument('-k', '--keyword', action="store", required=False, help="Substring for attaching files")

    return parser


def find_files(msg, keyword):
    files = []    
    all_files = [f for f in listdir(".") if isfile(join(".", f))]
    key_file = []

    if keyword == '':
        return msg

    for fl in all_files:
        # if keyword in fl:
        #     key_file.append(fl)
        filename = splitext(fl)
        if filename[1] == '.txt':
            with open(fl) as f:
                if keyword in f.read():
                    key_file.append(fl)
    
    for f in key_file or []:
        with open(f, "rb") as fl:
            part = MIMEApplication(fl.read(), Name=basename(f))
        part['Content-Disposition'] = 'attachment; filename="%s"' % basename(f)
        msg.attach(part)

    return msg
    

def send_mail(msg, msg_to_sent,  smtphost, from_mail, from_pass, to_mail, keyword):
    msg['From'] = from_mail
    msg['To'] = to_mail
    msg['Subject'] = "Test for labwork #5"
    msg.attach(MIMEText(msg_to_sent, 'plain'))

    msg = find_files(msg, keyword)

    server = smtplib.SMTP(smtphost[0], smtphost[1])
    server.starttls()
    server.login(from_mail, from_pass)
    res = server.sendmail(msg['From'], msg['To'], msg.as_string())
    server.quit()
    print("Successfully sent email message to %s:" % (msg['To']))


def main():
    parser = get_args()
    args = parser.parse_args(sys.argv[1:])
    msg = MIMEMultipart()
    msg_to_sent = "Hello world!"
    smtphost = ["smtp.yandex.ru", 25]

    if args.to_mail is not None and args.from_mail is not None and args.from_pass is not None and args.keyword is not None:
        send_mail(msg, msg_to_sent, smtphost, args.from_mail, args.from_pass, args.to_mail, args.keyword)
    else:
        send_mail(msg, msg_to_sent, smtphost, args.from_mail, args.from_pass, args.to_mail, '')


if __name__ == '__main__':
    main()