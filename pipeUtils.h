#ifndef KR_PIPE_UTILS_H
#define KR_PIPE_UTILS_H

void closePipes(int first, int second);
void exitWithError(char *errMessage, int readerPipe, int writerPipe);
void exitWithSuccess(char *message, int readerPipe, int writerPipe);

#endif //KR_PIPE_UTILS_H