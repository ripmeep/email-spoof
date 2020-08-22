#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <time.h>

#include <curl/curl.h>

#define MAX_EMAIL_LENGTH 256


struct time_format {
/* Thu Jul  2 03:11:57 2020 */
      char * ctime_str;
      char day[4];
      char month[32];

      int second;
      int minute;
      int hour;
      int date;
      int year;
};

struct upload_status {
      char ** payload_text;
      int lines_read;
};

typedef struct __Email {
      char from_address[MAX_EMAIL_LENGTH];
      struct curl_slist * recipients;
      char crecipients[MAX_EMAIL_LENGTH];
      size_t nrecipients;

      char subject[64];
      char * body;
      char * time;
} Email;


void newline(int n) {
      for (int i = 0; i < n; ++i)
            putchar('\n');
}

static char * curtime(struct time_format * tf) {
      time_t t;
      time(&t);

      char * c = ctime(&t);

      tf->ctime_str = c;

      sscanf(tf->ctime_str, "%s %s  %d %d:%d:%d %d\n\n",
            tf->day,
            tf->month,
            &tf->date,
            &tf->hour,
            &tf->minute,
            &tf->second,
            &tf->year
      );

      return c;
}

char * get_input_by_line(int color) {
      char * input = (char*)malloc(sizeof(char)*1);
      char line[256];
      int lines = 0;
      size_t size = 0;
      size_t line_len = 0;

      while (1) {
            memset(line, '\0', sizeof(line));
            printf("> \033[01;3%dm", color);
            fgets(line, sizeof(line), stdin);
            printf("\033[0m");

            if (!strcmp(line, "END\n"))
                  break;

            line_len = strlen(line);

            input = realloc(input, size + strlen(line));
            snprintf(input + size, line_len + 1, "%s", line);

            size += line_len;
      }

      return input;
}

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) {
      struct upload_status *upload_ctx = (struct upload_status *)userp;
      const char *data;

      if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
            return 0;
      }

      data = upload_ctx->payload_text[upload_ctx->lines_read];

      if(data) {
            size_t len = strlen(data);
            memcpy(ptr, data, len);
            upload_ctx->lines_read++;

            return len;
      }

      return 0;
}

void construct_payload(Email * e, struct upload_status * ctx) {
	size_t bytew, buf_size, new_size;

	ctx->payload_text = malloc(16);

	buf_size = 512;

	for (int i = 0; i < 16; ++i) {
		ctx->payload_text[i] = (char*)malloc(buf_size);
	}


	new_size = buf_size + strlen(e->time);
	ctx->payload_text[0] = (char*)realloc(ctx->payload_text[0], new_size);
	snprintf(ctx->payload_text[0], new_size, "Date: %s +1100\r\n", e->time);

	new_size = buf_size + strlen(e->crecipients);
	ctx->payload_text[1] = realloc(ctx->payload_text[1], new_size);
	snprintf(ctx->payload_text[1], new_size, "To: %s\r\n", e->crecipients);

	new_size = buf_size + strlen(e->from_address);
	ctx->payload_text[2] = (char*)realloc(ctx->payload_text[2], new_size);
	snprintf(ctx->payload_text[2], new_size, "From: %s\r\n", e->from_address);

	new_size = buf_size + strlen(e->subject);
	ctx->payload_text[3] = (char*)realloc(ctx->payload_text[3], new_size);
	snprintf(ctx->payload_text[3], new_size, "Subject: %s\r\n\r\n", e->subject);

	new_size = buf_size + strlen(e->body);
	ctx->payload_text[4] = (char*)realloc(ctx->payload_text[4], new_size);
	snprintf(ctx->payload_text[4], buf_size, "%s\r\n", e->body);

	ctx->payload_text[5] = NULL;
}

void init_email(Email * e) {
      e->recipients = NULL;
      e->nrecipients = 0;
      e->body = NULL;
}

int main(int argc, char ** argv) {
      if (argc < 2) {
            fprintf(stderr, "Usage:   %s [EMAIL RECIPIENT]\nExample: %s example@gmail.com\n", argv[0], argv[0]);

            return -1;
      }

      newline(1);

      CURL * curl;
      CURLcode res;
      Email email;

      init_email(&email);

      struct upload_status upload_ctx;

      upload_ctx.lines_read = 0;

      printf("\033[01;37mAdded Recipient -> \033[01;32m%s\033[0m\n", argv[1]);

      email.recipients = curl_slist_append(email.recipients, argv[1]);
      strncpy(email.crecipients, argv[1], MAX_EMAIL_LENGTH);

      newline(2);

      printf("\033[01;37mFROM ADDRESS> ");
      fgets(email.from_address, MAX_EMAIL_LENGTH, stdin);
      printf("\033[0m");

      printf("\033[01;37mEMAIL SUBJECT> ");
      fgets(email.subject, MAX_EMAIL_LENGTH, stdin);
      printf("\033[0m");

      email.from_address[strlen(email.from_address)-1] = '\0';
      email.subject[strlen(email.subject)-1] = '\0';

      newline(2);

      printf("ADD THE BODY OF THE EMAIL\nPRESS RETURN TO START A NEW LINE\nWHEN FINISHED, TYPE 'END' AND HIT RETURN TO FINISH THE EMAIL BODY\n\n\n");

      email.body = get_input_by_line(6);

      newline(2);

      printf("Full Email:\n\n");
      printf("From: \033[01;35m%s\033[0m\n", email.from_address);
      printf("To: ");

      for (int i = 1; i < argc; ++i) {
            printf("\033[01;32m%s\033[0m", argv[i]);

            if (i != argc - 1)
                  printf(", ");
      }

      printf("\nSubject: \033[01;33m%s\n\n", email.subject);
      printf("\033[01;36m%s\033[0m", email.body);

      newline(2);

      printf("PRESS RETURN TO SEND...");
      getchar();


      struct time_format tf;

      size_t size = 2048 + strlen(email.body);

      email.time = curtime(&tf);

      construct_payload(&email, &upload_ctx);

      curl = curl_easy_init();

      if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "smtp://localhost");
            curl_easy_setopt(curl, CURLOPT_MAIL_FROM, email.from_address);
            curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, email.recipients);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
            curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

            res = curl_easy_perform(curl);

            if(res != CURLE_OK)
                  fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));

      }

      curl_slist_free_all(email.recipients);
      curl_easy_cleanup(curl);

      return 0;
}
