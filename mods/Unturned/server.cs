using Rocket.Core.Plugins;
using SDG.Unturned;
using Steamworks;
using UnityEngine;
using System.Collections;
using System.Collections.Concurrent;
using System.Text;

public class MessageServerPlugin : RocketPlugin
{
    private bool isRunning;
    private Coroutine messageCoroutine;
    private Coroutine heartbeatCoroutine;
    private const int CHANNEL_ID = 69;
    private const uint MAX_MESSAGE_SIZE = 1024;
    private const float HEARTBEAT_INTERVAL = 5f; // Send heartbeat every 5 seconds

    private ConcurrentQueue<(CSteamID senderId, string message)> Queue1 = new ConcurrentQueue<(CSteamID, string)>();
    private ConcurrentQueue<(CSteamID senderId, string message)> Queue2 = new ConcurrentQueue<(CSteamID, string)>();

    private CSteamID firstPlayerSteamID = CSteamID.Nil;
    private CSteamID secondPlayerSteamID = CSteamID.Nil;

    protected override void Load()
    {
        base.Load();
        Rocket.Core.Logging.Logger.Log("MessageServer Loaded!");

        Provider.onServerConnected += OnPlayerConnected;
        Provider.onServerDisconnected += OnPlayerDisconnected;

        isRunning = true;
        StartCoroutine(ReceiveMessages());
        messageCoroutine = StartCoroutine(ProcessMessages());
        heartbeatCoroutine = StartCoroutine(SendHeartbeat());
    }

    protected override void Unload()
    {
        base.Unload();
        Rocket.Core.Logging.Logger.Log("MessageServer Unloaded!");

        Provider.onServerConnected -= OnPlayerConnected;
        Provider.onServerDisconnected -= OnPlayerDisconnected;

        isRunning = false;

        if (messageCoroutine != null)
        {
            StopCoroutine(messageCoroutine);
            messageCoroutine = null;
        }

        if (heartbeatCoroutine != null)
        {
            StopCoroutine(heartbeatCoroutine);
            heartbeatCoroutine = null;
        }
    }

    private void OnPlayerConnected(CSteamID steamID)
    {
        Rocket.Core.Logging.Logger.Log($"Player connected: {steamID}");

        if (firstPlayerSteamID == CSteamID.Nil)
        {
            firstPlayerSteamID = steamID;
            Rocket.Core.Logging.Logger.Log($"First player connected: {steamID}");
        }
        else if (secondPlayerSteamID == CSteamID.Nil)
        {
            secondPlayerSteamID = steamID;
            Rocket.Core.Logging.Logger.Log($"Second player connected: {steamID}");
        }
    }

    private void OnPlayerDisconnected(CSteamID steamID)
    {
        Rocket.Core.Logging.Logger.Log($"Player disconnected: {steamID}");

        if (steamID == firstPlayerSteamID)
        {
            firstPlayerSteamID = CSteamID.Nil;
            Rocket.Core.Logging.Logger.Log("First player disconnected.");
        }
        else if (steamID == secondPlayerSteamID)
        {
            secondPlayerSteamID = CSteamID.Nil;
            Rocket.Core.Logging.Logger.Log("Second player disconnected.");
        }
    }

    private IEnumerator ReceiveMessages()
    {
        WaitForSeconds wait = new WaitForSeconds(0.01f);
        byte[] messageBuffer = new byte[MAX_MESSAGE_SIZE];
        uint messageSize;
        CSteamID senderId;

        while (isRunning)
        {
            if (Provider.clients != null)
            {
                foreach (SteamPlayer player in Provider.clients)
                {
                    if (player?.playerID?.steamID == null) continue;

                    while (SteamGameServerNetworking.IsP2PPacketAvailable(out messageSize, CHANNEL_ID))
                    {
                        if (messageSize > MAX_MESSAGE_SIZE)
                        {
                            Rocket.Core.Logging.Logger.LogError($"Message too large: {messageSize} bytes");
                            continue;
                        }

                        if (SteamGameServerNetworking.ReadP2PPacket(messageBuffer, MAX_MESSAGE_SIZE, out messageSize, out senderId, CHANNEL_ID))
                        {
                            string message = Encoding.UTF8.GetString(messageBuffer, 0, (int)messageSize).TrimEnd('\0');

                            // Ignore heartbeat messages
                            if (message == "heartbeat") continue;

                            if (senderId == firstPlayerSteamID)
                            {
                                Queue1.Enqueue((senderId, message));
                                Rocket.Core.Logging.Logger.Log($"[RECEIVED - Queue1] {senderId}: {message}");
                            }
                            else
                            {
                                Queue2.Enqueue((senderId, message));
                                Rocket.Core.Logging.Logger.Log($"[RECEIVED - Queue2] {senderId}: {message}");
                            }
                        }
                    }
                }
            }

            yield return wait;
        }
    }

    private IEnumerator ProcessMessages()
    {
        WaitForSeconds wait = new WaitForSeconds(0.01f);

        while (isRunning)
        {
            // Process messages in Queue1
            while (Queue1.TryDequeue(out var messageData))
            {
                CSteamID senderId = messageData.senderId;
                string message = messageData.message;
                Rocket.Core.Logging.Logger.Log($"[PROCESSING - Queue1] {senderId}: {message}");

                byte[] messageBytes = Encoding.UTF8.GetBytes(message);
                if (secondPlayerSteamID != CSteamID.Nil)
                {
                    SteamGameServerNetworking.SendP2PPacket(
                        secondPlayerSteamID,
                        messageBytes,
                        (uint)messageBytes.Length,
                        EP2PSend.k_EP2PSendReliable,
                        CHANNEL_ID
                    );
                }
            }

            // Process messages in Queue2
            while (Queue2.TryDequeue(out var messageData))
            {
                CSteamID senderId = messageData.senderId;
                string message = messageData.message;
                Rocket.Core.Logging.Logger.Log($"[PROCESSING - Queue2] {senderId}: {message}");

                byte[] messageBytes = Encoding.UTF8.GetBytes(message);
                if (firstPlayerSteamID != CSteamID.Nil)
                {
                    SteamGameServerNetworking.SendP2PPacket(
                        firstPlayerSteamID,
                        messageBytes,
                        (uint)messageBytes.Length,
                        EP2PSend.k_EP2PSendReliable,
                        CHANNEL_ID
                    );
                }
            }

            yield return wait;
        }
    }

    private IEnumerator SendHeartbeat()
    {
        WaitForSeconds wait = new WaitForSeconds(HEARTBEAT_INTERVAL);
        byte[] heartbeatBytes = Encoding.UTF8.GetBytes("heartbeat");

        while (isRunning)
        {
            if (firstPlayerSteamID != CSteamID.Nil)
            {
                SteamGameServerNetworking.SendP2PPacket(
                    firstPlayerSteamID,
                    heartbeatBytes,
                    (uint)heartbeatBytes.Length,
                    EP2PSend.k_EP2PSendReliable,
                    CHANNEL_ID
                );
            }

            if (secondPlayerSteamID != CSteamID.Nil)
            {
                SteamGameServerNetworking.SendP2PPacket(
                    secondPlayerSteamID,
                    heartbeatBytes,
                    (uint)heartbeatBytes.Length,
                    EP2PSend.k_EP2PSendReliable,
                    CHANNEL_ID
                );
            }

            yield return wait;
        }
    }
}