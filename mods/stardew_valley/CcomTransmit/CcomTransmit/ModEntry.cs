using System;
using System.IO;
using StardewModdingAPI;
using StardewModdingAPI.Events;

namespace CcomTransmit
{
    class ModMessage
    {
        public int len;
        public byte[] buf;

        public ModMessage()
        {
            this.len = 0;
            this.buf = new byte[32];
        }
    }

    /// <summary>The mod entry point.</summary>
    internal sealed class ModEntry : Mod
    {
        private bool tx = false;
        private bool dbg = false;
        private FileStream fsr;
        private FileStream fsw;

        public override void Entry(IModHelper helper)
        {
            fsr = new FileStream("/tmp/ccom_send", FileMode.Open, FileAccess.ReadWrite, FileShare.ReadWrite);
            fsw = new FileStream("/tmp/ccom_recv", FileMode.Open, FileAccess.ReadWrite, FileShare.ReadWrite);

            helper.Events.Multiplayer.ModMessageReceived += this.OnModMessageReceived;
            helper.Events.GameLoop.UpdateTicked += this.OnUpdateTickedSendModMessage;
            helper.Events.Multiplayer.PeerConnected += this.OnPeerConnected;
            helper.ConsoleCommands.Add("ccom_tx", "Toggle ccom transmit.\n\nUsage: ccom_tx <bool>\n", this.toggle);
            helper.ConsoleCommands.Add("ccom_dbg", "Toggle ccom debug messages.\n\nUsage ccom_dbg <bool>\n", this.debug);
        }

        private void toggle(string command, string[] args)
        {
            this.tx = bool.Parse(args[0]);
            if (this.dbg) this.Monitor.Log($"ccom_tx -> {this.tx.ToString()}", LogLevel.Info);
        }

        private void debug(string command, string[] args)
        {
            this.dbg = bool.Parse(args[0]);
            if (this.dbg) this.Monitor.Log($"ccom_dbg -> {this.dbg.ToString()}", LogLevel.Info);
        }

        private void OnUpdateTickedSendModMessage(object sender, EventArgs e)
        {
            if (!this.tx) return;
            ModMessage message = new ModMessage();
            message.len = fsr.Read(message.buf, 0, message.buf.Length);
            if (message.len <= 0) return;
            this.Helper.Multiplayer.SendMessage(message, "ccom_msg");
            if (this.dbg) this.Monitor.Log($"{message.buf.ToString()} {message.len.ToString()} bytes.", LogLevel.Info);
        }

        private void OnModMessageReceived(object sender, ModMessageReceivedEventArgs e)
        {
            ModMessage message = e.ReadAs<ModMessage>();
            fsw.Write(message.buf, 0, message.len);
            fsw.Flush(true);
            if (this.dbg) this.Monitor.Log($"{message.buf.ToString()}", LogLevel.Info);
        }

        private void OnPeerConnected(object sender, PeerConnectedEventArgs e)
        {
            IMultiplayerPeer peer = e.Peer;
            if (!peer.HasSmapi) return;

            this.Monitor.Log($"Found player {peer.PlayerID} running Stardew Valley {peer.GameVersion} " +
                $"and SMAPI {peer.ApiVersion} on {peer.Platform}.", LogLevel.Info);
        }

    }
}