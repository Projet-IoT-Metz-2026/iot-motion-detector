import { Card, CardContent, CardHeader, CardTitle } from '../ui/card';
import { Button } from '../ui/button';
import { Input } from '../ui/input';
import { Label } from '../ui/label';
import { Textarea } from '../ui/textarea';
import { Copy, Eye, EyeOff, RefreshCw } from 'lucide-react';
import { useState } from 'react';

export function ConfigurationPage() {
  const [showKeys, setShowKeys] = useState(false);

  return (
    <div className="space-y-6">
      <div>
        <h2 className="text-gray-900 dark:text-white mb-1">Configuration</h2>
        <p className="text-gray-600 dark:text-gray-400">Paramètres Azure IoT Hub et DPS</p>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* IoT Hub Configuration */}
        <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
          <CardHeader>
            <CardTitle className="text-gray-900 dark:text-white">Azure IoT Hub</CardTitle>
          </CardHeader>
          <CardContent className="space-y-4">
            <div>
              <Label className="text-gray-300">Nom IoT Hub</Label>
              <Input 
                defaultValue="iot-detector-hub-2025" 
                className="bg-gray-700 border-gray-600 text-white mt-1"
              />
            </div>
            <div>
              <Label className="text-gray-300">Hostname</Label>
              <Input 
                defaultValue="iot-detector-hub-2025.azure-devices.net" 
                className="bg-gray-700 border-gray-600 text-white mt-1"
              />
            </div>
            <div>
              <Label className="text-gray-300">Clé d'accès primaire</Label>
              <div className="flex gap-2 mt-1">
                <Input 
                  type={showKeys ? "text" : "password"}
                  defaultValue="aB3dEf7gH9jKlM2nP5qRsT8vWxY1zA4cD6eF9gH2jK"
                  className="bg-gray-700 border-gray-600 text-white flex-1"
                />
                <Button 
                  variant="outline" 
                  size="icon"
                  onClick={() => setShowKeys(!showKeys)}
                  className="border-gray-600 text-gray-300 hover:bg-gray-700"
                >
                  {showKeys ? <EyeOff className="h-4 w-4" /> : <Eye className="h-4 w-4" />}
                </Button>
                <Button 
                  variant="outline" 
                  size="icon"
                  className="border-gray-600 text-gray-300 hover:bg-gray-700"
                >
                  <Copy className="h-4 w-4" />
                </Button>
              </div>
            </div>
            <div>
              <Label className="text-gray-300">Région</Label>
              <Input 
                defaultValue="West Europe" 
                className="bg-gray-700 border-gray-600 text-white mt-1"
              />
            </div>
            <Button className="w-full bg-blue-600 hover:bg-blue-700">
              Tester la connexion
            </Button>
          </CardContent>
        </Card>

        {/* DPS Configuration */}
        <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
          <CardHeader>
            <CardTitle className="text-gray-900 dark:text-white">Device Provisioning Service (DPS)</CardTitle>
          </CardHeader>
          <CardContent className="space-y-4">
            <div>
              <Label className="text-gray-300">ID Scope</Label>
              <Input 
                defaultValue="0ne00ABC123" 
                className="bg-gray-700 border-gray-600 text-white mt-1"
              />
            </div>
            <div>
              <Label className="text-gray-300">Endpoint global</Label>
              <Input 
                defaultValue="global.azure-devices-provisioning.net" 
                className="bg-gray-700 border-gray-600 text-white mt-1"
              />
            </div>
            <div>
              <Label className="text-gray-300">Clé primaire</Label>
              <div className="flex gap-2 mt-1">
                <Input 
                  type={showKeys ? "text" : "password"}
                  defaultValue="zY8xW6vU4tS2rQ1pO9nM7lK5jH3gF1dC"
                  className="bg-gray-700 border-gray-600 text-white flex-1"
                />
                <Button 
                  variant="outline" 
                  size="icon"
                  className="border-gray-600 text-gray-300 hover:bg-gray-700"
                >
                  <Copy className="h-4 w-4" />
                </Button>
              </div>
            </div>
            <div>
              <Label className="text-gray-300">Type d'attestation</Label>
              <Input 
                defaultValue="X.509 Certificates" 
                className="bg-gray-700 border-gray-600 text-white mt-1"
                readOnly
              />
            </div>
            <Button className="w-full bg-blue-600 hover:bg-blue-700">
              <RefreshCw className="h-4 w-4 mr-2" />
              Synchroniser appareils
            </Button>
          </CardContent>
        </Card>

        {/* Certificates */}
        <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700 lg:col-span-2">
          <CardHeader>
            <CardTitle className="text-gray-900 dark:text-white">Certificats X.509</CardTitle>
          </CardHeader>
          <CardContent className="space-y-4">
            <div>
              <Label className="text-gray-300">Certificat CA racine</Label>
              <Textarea 
                defaultValue="-----BEGIN CERTIFICATE-----
MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ
RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD
VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX
-----END CERTIFICATE-----"
                className="bg-gray-700 border-gray-600 text-white mt-1 h-32"
                readOnly
              />
            </div>
            <div className="flex gap-2">
              <Button variant="outline" className="border-gray-600 text-gray-300 hover:bg-gray-700">
                Télécharger certificat
              </Button>
              <Button variant="outline" className="border-gray-600 text-gray-300 hover:bg-gray-700">
                Générer nouveau certificat
              </Button>
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  );
}
