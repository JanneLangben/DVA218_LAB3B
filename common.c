//
// Created by quadragoon on 2019-05-06.
//

#include "common.h"


#define CRASHWITHERROR(message) perror(message);exit(EXIT_FAILURE)
#define LISTENING_PORT 23456
#define PACKET_BUFFER_SIZE 2048

int InitializeSocket()
{
    int socket_fd = 0;
    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        CRASHWITHERROR("socket() failed");
    }
    else
        return socket_fd;
}

ssize_t SendMessage(int socket_fd, const char* dataBuffer, int length, const struct sockaddr_in* receiverAddress, unsigned int addressLength)
{
    int retval = sendto(socket_fd, dataBuffer, length, MSG_CONFIRM, (struct sockaddr*)receiverAddress, addressLength);
    if (retval < 0)
    {
        CRASHWITHERROR("SendMessage() failed");
    }
    else
        return retval;
}

ssize_t ReceiveMessage(int socket_fd, char* packetBuffer, struct sockaddr_in* senderAddress, unsigned int* addressLength)
{
    int retval = recvfrom(socket_fd, packetBuffer, PACKET_BUFFER_SIZE, MSG_WAITALL, (struct sockaddr*)senderAddress, addressLength);
    if (retval < 0)
    {
        CRASHWITHERROR("SendMessage() failed");
    }
    else
        return retval;
}

ssize_t SendPacket(int socket_fd, packet* packetToSend, const struct sockaddr_in* receiverAddress, unsigned int addressLength)
{
    packetToSend->checksum = 0;
    packetToSend->checksum = (CalculateChecksum(packetToSend) ^ 65535u);
    int packetLength = PACKET_HEADER_LENGTH + packetToSend->dataLength;
    int retval = sendto(socket_fd, packetToSend, packetLength, MSG_CONFIRM, (struct sockaddr*)receiverAddress, addressLength);
    if (retval < 0)
    {
        CRASHWITHERROR("SendMessage() failed");
    }
    else
        return retval;
}

ssize_t ReceivePacket(int socket_fd, packet* packetBuffer, struct sockaddr_in* senderAddress, unsigned int* addressLength)
{
    int retval = recvfrom(socket_fd, packetBuffer, sizeof(packet), MSG_WAITALL, (struct sockaddr*)senderAddress, addressLength);
    if (retval < 0)
    {
        CRASHWITHERROR("ReceivePacket() failed");
    }
    else
    {
        unsigned short checksum = CalculateChecksum(packetBuffer);
        if (checksum == 65535u)
            return retval;
        else
        {
            printf("ReceivePacket() failed: checksum incorrect\nExpected 65535, got %d\n", checksum);
            exit(EXIT_FAILURE);
        }
    }
}

// Sets a flag in a packet to the specified value
// Returns 1 if the flag was changed, 0 if the flag remains the same, and -1 on error.
int SetPacketFlag(packet* packet, uint flag, int value)
{
    // First we do a bit of sanity checking on values
    if (flag > 128)
        return -1;
    if (value < 0 || value > 1)
        return -1;

    int originalValue = packet->flags & flag;
    if (originalValue == flag * value)
        return 0; // The flag is already set how we want it. Nothing was changed.
    else
    {
        if (originalValue == 0)
            packet->flags = (packet->flags | flag);
        else if (originalValue >= 1)
        {
            packet->flags = (packet->flags | flag);
            packet->flags = (packet->flags - flag);
        }
        else
            return -1; // Something went horribly wrong, the original value is out of bounds somehow???

        return 1; // The flag was changed, so we're returning 1
    }
}

unsigned short CalculateChecksum(const packet* packet)
{
    unsigned int total = 0;
    byte* packetBytes = (byte*)packet;

    unsigned int numBytesInPacket = PACKET_HEADER_LENGTH + packet->dataLength;
    for (int i = 0; i < numBytesInPacket; i+=2)
    {
        if (i == numBytesInPacket - 1)
            total += packetBytes[i] * 256;
        else
            total += (unsigned short)packetBytes[i];
    }

    while (total > 65535)
    {
        unsigned short last16 = total & 65535u;
        total = (total >> 16u);
        total += last16;
    }

    return total;
}