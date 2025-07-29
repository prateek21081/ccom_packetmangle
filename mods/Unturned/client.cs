using SDG.Unturned;
using Steamworks;
using UnityEngine;
using System.Collections;
using System;
using System.IO;
using System.Threading;

namespace MessageModule
{
    public class MessageClient : MonoBehaviour
    {
        private bool isRunning;
        private Coroutine sendCoroutine;
        private const int CHANNEL_ID = 69;
        private const string SEND_FILE_PATH = "ccom_send.txt";
        private const string RECEIVE_FILE_PATH = "ccom_receive.txt";
        private Callback<P2PSessionRequest_t> p2pSessionRequest;
        private CallResult<P2PSessionConnectFail_t> p2pConnectFail;
        private FileSystemWatcher sendFileWatcher;
        private readonly object fileLock = new object();
        private long lastReadPosition = 0;
       
        // Optimization: Pre-allocate buffers
        private const int BUFFER_SIZE = 8192;
        private readonly byte[] messageBytes = new byte[BUFFER_SIZE];
        private readonly byte[] tempBuffer = new byte[BUFFER_SIZE];

        void Start()
        {
            Debug.Log("MessageClient Started!");
            InitializeFileWatchers();
            isRunning = true;
            sendCoroutine = StartCoroutine(ProcessMessages());
            p2pSessionRequest = Callback<P2PSessionRequest_t>.Create(OnP2PSessionRequest);
            p2pConnectFail = CallResult<P2PSessionConnectFail_t>.Create(OnP2PConnectionFailed);
        }

        void OnDestroy()
        {
            isRunning = false;
            if (sendCoroutine != null) StopCoroutine(sendCoroutine);
            CleanupFileWatchers();
        }

        private void InitializeFileWatchers()
        {
            // Ensure files exist
            if (!File.Exists(RECEIVE_FILE_PATH))
            {
                File.WriteAllBytes(RECEIVE_FILE_PATH, new byte[0]);
            }
            if (!File.Exists(SEND_FILE_PATH))
            {
                File.WriteAllBytes(SEND_FILE_PATH, new byte[0]);
            }

            // Initialize send file watcher
            sendFileWatcher = new FileSystemWatcher(Path.GetDirectoryName(Path.GetFullPath(SEND_FILE_PATH)));
            sendFileWatcher.Filter = Path.GetFileName(SEND_FILE_PATH);
            sendFileWatcher.NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.Size;
            sendFileWatcher.Changed += OnSendFileChanged;
            sendFileWatcher.EnableRaisingEvents = true;

            // Get initial file size
            var fileInfo = new FileInfo(SEND_FILE_PATH);
            lastReadPosition = fileInfo.Length;
        }

        private void CleanupFileWatchers()
        {
            if (sendFileWatcher != null)
            {
                sendFileWatcher.EnableRaisingEvents = false;
                sendFileWatcher.Changed -= OnSendFileChanged;
                sendFileWatcher.Dispose();
            }
        }

        private void OnSendFileChanged(object sender, FileSystemEventArgs e)
        {
            if (e.ChangeType == WatcherChangeTypes.Changed)
            {
                ProcessFileChanges();
            }
        }

        private void ProcessFileChanges()
        {
            if (!isRunning || !Provider.isConnected || Provider.server == null) return;

            lock (fileLock)
            {
                try
                {
                    using (var fileStream = new FileStream(SEND_FILE_PATH, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
                    {
                        if (fileStream.Length <= lastReadPosition) return;

                        fileStream.Position = lastReadPosition;
                        long bytesToRead = fileStream.Length - lastReadPosition;
                       
                        // Read all new bytes from the file
                        byte[] newData = new byte[bytesToRead];
                        int totalBytesRead = 0;
                       
                        while (totalBytesRead < bytesToRead)
                        {
                            int bytesRead = fileStream.Read(newData, totalBytesRead, (int)(bytesToRead - totalBytesRead));
                            if (bytesRead == 0) break; // End of file
                            totalBytesRead += bytesRead;
                        }

                        lastReadPosition = fileStream.Position;

                        // Send the exact bytes that were added to the file
                        if (totalBytesRead > 0)
                        {
                            SendP2PMessage(newData, totalBytesRead);
                        }
                    }
                }
                catch (IOException ex)
                {
                    Debug.LogError($"Error reading file: {ex.Message}");
                }
            }
        }

        private void SendP2PMessage(byte[] data, int length)
        {
            // Send the raw bytes without any encoding/decoding
            SteamNetworking.SendP2PPacket(
                Provider.server,
                data,
                (uint)length,
                EP2PSend.k_EP2PSendReliable,
                CHANNEL_ID
            );
           
            // For debug purposes, show hex representation of sent data
            string hexData = BitConverter.ToString(data, 0, Math.Min(length, 50));
            if (length > 50) hexData += "...";
            Debug.Log($"[SENT TO SERVER] {length} bytes: {hexData}");
        }

        private void OnP2PSessionRequest(P2PSessionRequest_t request)
        {
            SteamNetworking.AcceptP2PSessionWithUser(request.m_steamIDRemote);
        }

        private void OnP2PConnectionFailed(P2PSessionConnectFail_t result, bool failed)
        {
            Debug.LogError($"P2P Connection Failed with {result.m_steamIDRemote}");
        }

        void Update()
        {
            ProcessIncomingMessages();
        }

        private void ProcessIncomingMessages()
        {
            while (SteamNetworking.IsP2PPacketAvailable(out uint packetSize, CHANNEL_ID))
            {
                if (packetSize > BUFFER_SIZE)
                {
                    Debug.LogError($"Received packet size {packetSize} exceeds buffer size {BUFFER_SIZE}");
                    continue;
                }

                if (SteamNetworking.ReadP2PPacket(messageBytes, packetSize, out uint bytesRead, out CSteamID remoteId, CHANNEL_ID))
                {
                    // Check if it's a heartbeat message (assuming heartbeat is UTF-8 encoded)
                    bool isHeartbeat = false;
                    if (bytesRead >= 9) // "heartbeat" is 9 bytes in UTF-8
                    {
                        try
                        {
                            string possibleHeartbeat = System.Text.Encoding.UTF8.GetString(messageBytes, 0, (int)bytesRead);
                            isHeartbeat = possibleHeartbeat.Trim().ToLower() == "heartbeat";
                        }
                        catch
                        {
                            // If UTF-8 decoding fails, it's not a heartbeat
                            isHeartbeat = false;
                        }
                    }

                    if (!isHeartbeat)
                    {
                        try
                        {
                            // Write the exact raw bytes to the receive file
                            byte[] dataToWrite = new byte[bytesRead];
                            Array.Copy(messageBytes, 0, dataToWrite, 0, (int)bytesRead);
                           
                            // Append raw bytes to file
                            using (var fileStream = new FileStream(RECEIVE_FILE_PATH, FileMode.Append, FileAccess.Write, FileShare.Read))
                            {
                                fileStream.Write(dataToWrite, 0, (int)bytesRead);
                            }
                           
                            // For debug purposes, show hex representation of received data
                            string hexData = BitConverter.ToString(dataToWrite, 0, Math.Min((int)bytesRead, 50));
                            if (bytesRead > 50) hexData += "...";
                            Debug.Log($"[RECEIVED FROM SERVER] {remoteId}: {bytesRead} bytes: {hexData}");
                        }
                        catch (IOException ex)
                        {
                            Debug.LogError($"Error writing to receive file: {ex.Message}");
                        }
                    }
                }
            }
        }

        private IEnumerator ProcessMessages()
        {
            WaitForSeconds wait = new WaitForSeconds(0.1f);
            while (isRunning)
            {
                if (Provider.isConnected && Provider.server != null)
                {
                    ProcessFileChanges();
                }
                yield return wait;
            }
        }
    }

    public class MessageClientLoader
    {
        private static GameObject gameObject;

        public static void Load()
        {
            gameObject = new GameObject("MessageClient");
            gameObject.AddComponent<MessageClient>();
            GameObject.DontDestroyOnLoad(gameObject);
        }

        public static void Unload()
        {
            if (gameObject != null)
            {
                GameObject.Destroy(gameObject);
            }
        }
    }

    public class Module : SDG.Framework.Modules.IModuleNexus
    {
        public void initialize()
        {
            MessageClientLoader.Load();
        }

        public void shutdown()
        {
            MessageClientLoader.Unload();
        }
    }
}