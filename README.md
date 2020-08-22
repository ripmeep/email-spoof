# email-spoof
A simple email spoofer made in C and libcurl


# Setup

    apt-get install sendmail libcurl4-openssl-dev
    service sendmail start

    gcc -o email_spoof email_spoof.c -lcurl
    
    ./email_spoof [RECIPIENT EMAIL]
    
    #example
    
    ./email_spoof example@gmail.com
