import { Card, CardContent, CardHeader, CardTitle } from '../ui/card';
import { Button } from '../ui/button';
import { Avatar, AvatarFallback, AvatarImage } from '../ui/avatar';
import { Badge } from '../ui/badge';
import { Github, ExternalLink, Mail, Linkedin } from 'lucide-react';

const teamMembers = [
  {
    name: 'Anis Makhezer',
    role: 'Dev IoT / Azure',
    description: 'Spécialiste IoT et intégration Azure IoT Hub',
    avatar: 'AM',
    skills: ['ESP32', 'Azure IoT', 'MQTT', 'C++'],
    email: 'anis.makhezer@example.com',
  },
  {
    name: 'Abdelmalek Allouche',
    role: 'Dev Backend',
    description: 'Architecture backend et API Azure',
    avatar: 'AA',
    skills: ['.NET', 'Azure Functions', 'SQL', 'API REST'],
    email: 'abdelmalek.allouche@example.com',
  },
  {
    name: 'Melissa Oumoussa',
    role: 'UI/UX & Dev Front',
    description: 'Design et développement interface utilisateur',
    avatar: 'MO',
    skills: ['React', 'Tailwind', 'Figma', 'TypeScript'],
    email: 'melissa.oumoussa@example.com',
  },
];

export function TeamPage() {
  return (
    <div className="space-y-6">
      <div>
        <h2 className="text-gray-900 dark:text-white mb-1">Équipe / Projet</h2>
        <p className="text-gray-600 dark:text-gray-400">Membres de l'équipe et informations projet</p>
      </div>

      {/* Project Info */}
      <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
        <CardHeader>
          <CardTitle className="text-gray-900 dark:text-white">Projet IoT Detector</CardTitle>
        </CardHeader>
        <CardContent className="space-y-4">
          <p className="text-gray-300">
            Système de détection et supervision IoT basé sur ESP32 et Photon2, connecté à Azure IoT Hub 
            pour la collecte et l'analyse de données en temps réel.
          </p>
          <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
            <div>
              <p className="text-gray-500 dark:text-gray-400">Programme</p>
              <p className="text-gray-900 dark:text-white">M2 EEA</p>
            </div>
            <div>
              <p className="text-gray-500 dark:text-gray-400">Année</p>
              <p className="text-gray-900 dark:text-white">2024-2025</p>
            </div>
            <div>
              <p className="text-gray-500 dark:text-gray-400">Technologies</p>
              <p className="text-gray-900 dark:text-white">Azure IoT, ESP32, .NET</p>
            </div>
          </div>
          <div className="flex gap-3 pt-4">
            <Button className="bg-blue-600 hover:bg-blue-700">
              <Github className="h-4 w-4 mr-2" />
              GitHub Repository
            </Button>
            <Button variant="outline" className="border-gray-600 text-gray-300 hover:bg-gray-700">
              <ExternalLink className="h-4 w-4 mr-2" />
              Azure DevOps
            </Button>
            <Button variant="outline" className="border-gray-600 text-gray-300 hover:bg-gray-700">
              Voir architecture globale
            </Button>
          </div>
        </CardContent>
      </Card>

      {/* Team Members */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
        {teamMembers.map((member) => (
          <Card key={member.name} className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
            <CardHeader className="text-center">
              <div className="flex flex-col items-center gap-3">
                <Avatar className="h-20 w-20">
                  <AvatarImage src="" />
                  <AvatarFallback className="bg-blue-600 text-white">
                    {member.avatar}
                  </AvatarFallback>
                </Avatar>
                <div>
                  <CardTitle className="text-gray-900 dark:text-white">{member.name}</CardTitle>
                  <p className="text-blue-400 mt-1">{member.role}</p>
                </div>
              </div>
            </CardHeader>
            <CardContent className="space-y-4">
              <p className="text-gray-400 text-center">{member.description}</p>
              
              <div>
                <p className="text-gray-500 dark:text-gray-400 mb-2">Compétences</p>
                <div className="flex flex-wrap gap-2 justify-center">
                  {member.skills.map((skill) => (
                    <Badge key={skill} variant="outline" className="border-gray-600 text-gray-300">
                      {skill}
                    </Badge>
                  ))}
                </div>
              </div>

              <div className="flex gap-2 justify-center pt-2">
                <Button variant="ghost" size="icon" className="text-gray-400 hover:text-white hover:bg-gray-700">
                  <Mail className="h-4 w-4" />
                </Button>
                <Button variant="ghost" size="icon" className="text-gray-400 hover:text-white hover:bg-gray-700">
                  <Github className="h-4 w-4" />
                </Button>
                <Button variant="ghost" size="icon" className="text-gray-400 hover:text-white hover:bg-gray-700">
                  <Linkedin className="h-4 w-4" />
                </Button>
              </div>
            </CardContent>
          </Card>
        ))}
      </div>
    </div>
  );
}
