using Azure.Messaging.EventHubs; // On a besoin de ça maintenant
using Microsoft.Azure.Functions.Worker;
using Microsoft.Extensions.Logging;
using System;  // On a besoin de ça pour l'heure
using System.Text;

namespace IoTHubProcessor
{
    public class IoTHubProcessor
    {
        private readonly ILogger<IoTHubProcessor> _logger;

        public IoTHubProcessor(ILogger<IoTHubProcessor> logger)
        {
            _logger = logger;
        }

        // --- CHANGEMENT IMPORTANT ICI ---
        // On ne reçoit plus un simple `string[]`, mais un `EventData[]`.
        // `EventData` est un objet plus riche qui contient le message ET ses métadonnées (comme l'heure d'arrivée !)
        [Function(nameof(IoTHubProcessor))]
        public void Run([EventHubTrigger("%EventHubName%", Connection = "EventHubConnectionString")] EventData[] messages)
        {
            _logger.LogInformation($"--- Déclenchement avec {messages.Length} message(s) ---");

            foreach (var message in messages)
            {
                // 1. On récupère l'heure UTC à laquelle Azure a reçu le message.
                var messageUtcTime = message.EnqueuedTime.DateTime;

                // 2. On trouve le fuseau horaire de Paris (qui est UTC+2 en été).
                // Sur Windows, l'identifiant est "Romance Standard Time". Sur Linux/macOS, ce serait "Europe/Paris".
                var parisZone = TimeZoneInfo.FindSystemTimeZoneById("Romance Standard Time");

                // 3. On convertit l'heure UTC en heure de Paris.
                var parisTime = TimeZoneInfo.ConvertTimeFromUtc(messageUtcTime, parisZone);

                // 4. On récupère le corps du message (le JSON).
                string messageBody = Encoding.UTF8.GetString(message.EventBody.ToArray());
                
                // 5. On affiche tout !
                _logger.LogInformation($"✅ SUCCÈS : Message Reçu : {messageBody}");
                _logger.LogInformation($"   -> Heure du serveur (UTC): {messageUtcTime:HH:mm:ss} | Heure de Paris (UTC+2): {parisTime:HH:mm:ss}");
            }
        }
    }
}

