#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <curl/curl.h>

pthread_t tid[2];
pthread_mutex_t curlLock;
time_t now;
struct tm *timeInfo;

size_t write_data(void *contents, size_t size, size_t nmemb, void *userp)
{ 
  return size * nmemb;
}

void *work(CURL *curl)
{
	while (1)
	{
		CURLcode res;
		pthread_mutex_lock(&curlLock);
			
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{ \"content\": \"!work\" }");

		res = curl_easy_perform(curl);

		if(res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
					  curl_easy_strerror(res));
		}
					
		pthread_mutex_unlock(&curlLock);
		
		time(&now);
		timeInfo = localtime(&now);
		printf("[%d:%d] Work\n", timeInfo->tm_hour, timeInfo->tm_min);
		Sleep(3600000);
	}
	return NULL;
}

void *daily(CURL *curl)
{
	while (1)
	{
		CURLcode res;
	
		pthread_mutex_lock(&curlLock);
		
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{ \"content\": \"!daily\" }");

		res = curl_easy_perform(curl);

		if(res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
					  curl_easy_strerror(res));
		}	
			
		pthread_mutex_unlock(&curlLock);
		time(&now);
		timeInfo = localtime(&now);
		printf("[%d:%d] Daily\n", timeInfo->tm_hour, timeInfo->tm_min);
		Sleep(86400000);
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("usage: koronFarmer authToken channelId\n");
		exit(1);
	}
	
	CURL *curl;
	struct curl_slist *headers;
	 
	if (pthread_mutex_init(&curlLock, NULL) != 0)
	{
		printf("Failed to initialize mutex\n");
		return 1;
	}
	 
	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);
	 
	/* get a curl handle */
	curl = curl_easy_init();
	
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	
	if (curl) 
	{
		char buffer[128];
		sprintf(buffer, "Authorization: %s", argv[1]);
		headers = curl_slist_append(NULL, buffer);
		headers = curl_slist_append(headers, "Content-Type: application/json");
		
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); 
		
		sprintf(buffer, "https://discord.com/api/v9/channels/%s/messages", argv[2]);
		curl_easy_setopt(curl, CURLOPT_URL, buffer);  
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);

		int error;
		error = pthread_create(&(tid[0]), NULL, &work, curl);
		if (error != 0)
		{
			printf("Failed to create thread: %s\n", strerror(error));
		}
		error = pthread_create(&(tid[1]), NULL, &daily, curl);
		if (error != 0)
		{
			printf("Failed to create thread: %s\n", strerror(error));
		}

		pthread_join(tid[0], NULL);
		Sleep(1000);
		pthread_join(tid[1], NULL);
		pthread_mutex_destroy(&curlLock);

		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
	}
	
	curl_global_cleanup();
	return 0;
}