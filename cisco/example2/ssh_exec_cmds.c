#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libssh/libssh.h>

#define MAX_LINE 1024
#define MAX_CMDS 8192

int main(int argc, char **argv)
{
  // Map variables to argv positions 
  const char *host = argv[1];
  const char *user = argv[2];
  const char *password = argv[3];
  const char *file = argv[4];

  // Initialize SSH variables
  ssh_session session;
  int rc;

  // Create SSH session
  session = ssh_new();
  if (session == NULL) {
    fprintf(stderr, "Error creating SSH session\n");
    return EXIT_FAILURE;;
  }

  // Set SSH options
  //Uncomment to debug
  //int verbosity = SSH_LOG_PROTOCOL;
  //ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity); 
  ssh_options_set(session, SSH_OPTIONS_HOST, host); 
  ssh_options_set(session, SSH_OPTIONS_USER, user); 

  // Connect to SSH server
  rc = ssh_connect(session);
  if (rc != SSH_OK) {
    fprintf(stderr, "Error connecting to SSH server: %s\n", ssh_get_error(session));
    ssh_free(session);
    return EXIT_FAILURE;;
  }

  // Authenticate with password
  rc = ssh_userauth_password(session, NULL, password); 
  if (rc != SSH_AUTH_SUCCESS) {
    fprintf(stderr, "Error authenticating with password: %s\n", ssh_get_error(session));
    ssh_disconnect(session);
    ssh_free(session);
    return EXIT_FAILURE;
  }

  // Open channel for executing command
  ssh_channel channel;
  channel = ssh_channel_new(session);
  if (channel == NULL) {
    fprintf(stderr, "Error creating SSH channel\n");
    ssh_disconnect(session);
    ssh_free(session);
    return EXIT_FAILURE;
  }

  rc = ssh_channel_open_session(channel);
  if (rc != SSH_OK) {
    fprintf(stderr, "Error opening SSH channel: %s\n", ssh_get_error(session));
    ssh_channel_free(channel);
    ssh_disconnect(session);
    ssh_free(session);
    return EXIT_FAILURE;
  }

  char commands[MAX_CMDS];
  char line[MAX_LINE];
  FILE * fp = fopen(file, "r"); 
  if (!fp) {
    printf("Error openning file.....\n");
    return EXIT_FAILURE;
  }
  
  while (fgets(line, sizeof(line), fp) != NULL) {
    line[strcspn(line, "\r\n")] = 0;
    if (strlen(line) == 0) continue;
    strcat(commands, line);
    strcat(commands, " ; ");
  }
  //printf("%s", commands); //used to debug
  fclose(fp);

  //This is where commands get applied
  rc = ssh_channel_request_exec(channel, commands);
  if (rc != SSH_OK) {
    fprintf(stderr, "Error executing command: %s\n", ssh_get_error(session));
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(session);
    ssh_free(session);
    return EXIT_FAILURE;
  }

  // Read command output
  char buffer[2048];
  int nbytes;
  while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {
    fwrite(buffer, 1, nbytes, stdout);
  }

  // Close channel
  ssh_channel_send_eof(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);

  // Disconnect and free SSH session
  ssh_disconnect(session);
  ssh_free(session);
  printf("\n\n");
  return EXIT_SUCCESS;
}
