using System;
using System.IO;
using StardewModdingAPI;
using StardewModdingAPI.Events;

namespace CcomTransmit
{
    class ModMessage
    {
        public sbyte len;
        public byte[] buf;

        public ModMessage(byte size)
        {
            this.len = 0;
            this.buf = new byte[size];
        }
    }

    /// <summary>The mod entry point.</summary>
    internal sealed class ModEntry : Mod
    {
        private bool send = false;
        private bool recv = false;
        private bool debug = false;
        private FileStream fsr;
        private FileStream fsw;
        ModMessage message;

        public override void Entry(IModHelper helper)
        {
            message = new ModMessage(24);
            fsr = new FileStream("/tmp/ccom_send", FileMode.Open, FileAccess.ReadWrite, FileShare.ReadWrite);
            fsw = new FileStream("/tmp/ccom_recv", FileMode.Open, FileAccess.ReadWrite, FileShare.ReadWrite);

            helper.Events.Multiplayer.ModMessageReceived += this.OnModMessageReceived;
            helper.Events.GameLoop.UpdateTicked += this.OnUpdateTickedSendModMessage;
            helper.Events.Multiplayer.PeerConnected += this.OnPeerConnected;
            helper.ConsoleCommands.Add("ccom_send", "Toggle ccom transmit.\n\nUsage: ccom_send <bool>\n", this.set_send);
            helper.ConsoleCommands.Add("ccom_recv", "Toggle ccom recieve.\n\nUsage: ccom_recv <bool>\n", this.set_recv);
            helper.ConsoleCommands.Add("ccom_debug", "Toggle ccom debug messages.\n\nUsage ccom_dbg <bool>\n", this.set_debug);
        }

        private void set_send(string command, string[] args)
        {
            this.send = bool.Parse(args[0]);
            if (this.debug) this.Monitor.Log($"ccom_send -> {this.send.ToString()}", LogLevel.Info);
        }

        private void set_recv(string command, string[] args)
        {
            this.recv = bool.Parse(args[0]);
            if (this.debug) this.Monitor.Log($"ccom_recv -> {this.recv.ToString()}", LogLevel.Info);
        }

        private void set_debug(string command, string[] args)
        {
            this.debug = bool.Parse(args[0]);
            if (this.debug) this.Monitor.Log($"ccom_dbg -> {this.debug.ToString()}", LogLevel.Info);
        }

        private void OnUpdateTickedSendModMessage(object sender, UpdateTickedEventArgs e)
        {
            if (!e.IsMultipleOf(3) || !this.send) return;
            message.len = (sbyte)fsr.Read(message.buf, 0, message.buf.Length);
            if (message.len <= 0) return;
            this.Helper.Multiplayer.SendMessage(message, "ccom_msg");
            if (this.debug) this.Monitor.Log($"{message.buf.ToString()} {message.len.ToString()} bytes.", LogLevel.Info);
        }

        private void OnModMessageReceived(object sender, ModMessageReceivedEventArgs e)
        {
            if (!this.recv) return;
            ModMessage msg_recv = e.ReadAs<ModMessage>();
            fsw.Write(msg_recv.buf, 0, msg_recv.len);
            fsw.Flush(true);
            if (this.debug) this.Monitor.Log($"{msg_recv.buf.ToString()}", LogLevel.Info);
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